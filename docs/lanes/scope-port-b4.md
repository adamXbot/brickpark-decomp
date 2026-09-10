# Scope PORT-B4 — the front end comes up: sound bring-up, the frame loop, and where input stops

Branch `scope/PORT-B4`, from `feat/decomp-completion-next-steps-24a0d6`
@ `595aae91` (the PORT-B3 merge). Files owned: `portable/src/hostwin/dsound.c`,
`dinput.c`, `portable/src/browser/**`, `portable/hostwin/include/ll_host.h`
additions. `LEGOLAND/*.c` untouched.

**Headline: the game reaches its first front-end menu.** `PLAYER DETAILS`, the
eight-slot profile screen, renders complete — backdrop, the Lego-man artwork,
the eight `EMPTY` slot captions in the shim's own bitmap font, the pointer — and
the game's own loop runs it at **33.5 fps sustained**, which is the frame floor
the game itself enforces. Before this lane the run stopped at the title screen
in `RunGame`'s music wait.

**What is NOT working: input.** The shim delivers keyboard and mouse to the game
once per presented frame, measured; the front end does not act on any of it. The
evidence, and everything ruled out, is §5. It is not, as far as this lane can
show, a shim defect.

---

## 1. Why the title screen stopped, and why `DirectSoundCreate` had to succeed

PORT-B3 left the run spinning in

```c
/* gamemain.c 0x00459520 RunGame, line 370 */
while (g_music_disabled == 0) { PeekMessageA(&msg, 0, 0, 0, 0); Sleep(100); progress_tick(); }
```

16.7 million `Sleep(100)` trace lines in a 90-second headless run, and no exit.

`g_music_disabled` is written by exactly two pieces of code — sysstubs.c's
`InitMusicSystem` (0x00495a10) when `g_music_sys` is 0, which is what `-nomusic`
arranges (startup.c:254 `g_music_sys = FindCommandSwitch(cmdline, "-nomusic") == 0`),
and musicthread.c's `MusicThread` (0x00492db0) on every one of its failure
exits. Neither can run, because:

```c
/* lifecycle.c 0x004964f0 InitSoundSystem */
ok = InitSoundSampleSystem(g_snd_hwnd);
if (!ok) return ok;                      /* <-- returns HERE */
return InitMusicSystem(g_snd_hwnd) != 0;
```

PORT-B's reasoning for the failing `DirectSoundCreate` was sound as far as it
went — every *sample* entry point is guarded by `g_samples_ready`, so a null
`g_dsound` is safe. What it missed is that `InitSoundSystem` is not a sample
entry point. **A failing sample system makes the `-nomusic` switch
unreachable.** So the device has to succeed.

`InitSoundSampleSystem` (audio4.c 0x00492130) needs three calls to work —
`DirectSoundCreate`, `SetCooperativeLevel`, `GetCaps` — and then sets
`g_samples_ready = 1`, which un-guards the entire sample layer at once. So the
object is a real silent `IDirectSound`, not three stubs. Every `lpVtbl->` call
site on `g_dsound` or on a sound buffer in `LEGOLAND/*.c` was swept before it
was written; the list is in the file header of `portable/src/hostwin/dsound.c`.

### The play cursor is the part that could not be faked

Two loops in the game are driven by the play cursor, and both hang on a frozen
one:

```c
/* input2.c 0x004963f0 KLIBAUDIO_LockAVISoundBuffer */
if (hr == 0 && (status & DSBSTATUS_PLAYING)) {
    hr = GetCurrentPosition(buf, &play, &write);
    while (hr == 0 && play >= offset && play < offset + len)
        hr = GetCurrentPosition(buf, &play, &write);      /* spins */
}
```

```c
/* narration2.c 0x00498b40 PumpNarration */
do { ... if (--g_speech_blocks_ready == 0) { StopNarrationPlayback(); return 0; } }
while (!(GetCurrentPosition(..) == 0 && play in the next fill block));
```

The second is worse than it looks: the exit test on the counter is `== 0`, so a
cursor that never reaches the fill block drives `g_speech_blocks_ready` negative
and the loop never terminates. (`PumpNarration` is `sub_498b40`, the FIRST call
in `GameFrame` — gameframe.c:437 — so it runs every frame of the front end.)

Rather than special-case either loop, every buffer gets an honest cursor: wall
clock times the byte rate, wrapping if it was started looping and stopping at
the end if it was not. That is what a real card does with the speakers
unplugged, and both loops then terminate for the same reason they terminate on
Windows. `GetStatus` reporting 0 at the end of a one-shot is also what lets
`AutoKillSamples` (narration2.c 0x00496920) reap the instance.

Two results are read as numbers rather than HRESULTs and are not free to be
arbitrary:

* `Release` must return the **remaining reference count** — audiomisc.c:51 is
  literally `return buf->lpVtbl->Release(buf) == 0;`, so a final release that
  reports anything else makes `KLIBAUDIO_DestroyAVISoundBuffer` report failure.
* `GetFrequency` must read back the **format's** rate when none was set, because
  `GetSampleFrequency` (audio3.c 0x00492a60) returns it to the game and
  `SetSampleFrequency` scales it.

Duplicates share the PCM block, as on Windows: `StartPlayableSample`
(audio3.c 0x00492370) makes one duplicate per playable *instance*, so copying
would multiply the game's whole sound set by its polyphony.

`Lock` has one shape worth naming: `data2.c:676` uploads a sample with
`Lock(buf, 0, 0, &ptr1, &bytes1, 0, 0, DSBLOCK_ENTIREBUFFER)` and then
`memcpy(ptr1, conv, bytes1)` — so `bytes1` must come back as the **full** buffer
length or every sample is uploaded short. Three of the four call sites pass NULL
for the second (wrapped) region.

## 2. ole32, and the one thing the music-on path still needs

`CoInitialize` and `CoCreateInstance` are the program's only COM imports
(`portable/tools/win32_imports.txt`) and `MusicThread` is their only caller, so
they live in `dsound.c` rather than a file of their own. `CoCreateInstance`
reports `REGDB_E_CLASSNOTREG`, which takes the thread to its first `shutdown:`
rung — releasing nothing, because nothing has been created — and that rung sets
`g_music_ready = 0; g_music_disabled = 1;`. **The game tolerates it exactly as
designed; nothing is needed on the game's side.**

**But `MusicThread` never runs.** `kernel32.c`'s `CreateThread` refuses (the
port is single-threaded), `InitMusicSystem` reports success anyway on a null
thread handle — "A null thread handle is still reported as success when the
engine exists", sysstubs.c:394 — and nothing then writes `g_music_disabled`. So
**without `-nomusic` `RunGame` still waits for ever.**

Measured, with a throwaway (uncommitted, reverted) one-line change to
`CreateThread` that runs the start routine inline:

```
HOST CreateThread: running inline (PORT-B4 experiment)
HOST CoInitialize: no COM runtime; the result is discarded
HOST CoCreateInstance: REGDB_E_CLASSNOTREG (no DirectMusic)
...
TRAP GAME ODFError from llidb_odf.c          <-- the same place -nomusic reaches
```

**Owner: PORT-A (`portable/src/hostwin/kernel32.c`, this lane may not edit it).**
The fix is to run the start routine synchronously and return a non-null handle.
That is safe *for this program specifically*: `CreateThread` has exactly one
caller in the whole tree (sysstubs.c:399), and with `CoCreateInstance` failing
`MusicThread` sets its two flags and returns immediately — it does not reach its
message loop. If DirectMusic is ever real, this stops being safe and the loop
needs a different home.

## 3. The loop runs, and it runs at its ceiling

`?trace=1` now also shows fps, and it counts **presented** frames
(`ll_canvas.js`'s `llFrame`), not `requestAnimationFrame` — under ASYNCIFY it is
the game's own loop that decides when it presents, so that is the honest number.

Front end, `-nointro -nomusic`, ~3,700 frames: **33.5 fps rolling, 33.5 average.**
`FlipPrimary` and `PresentFlip` both enforce `while (timeGetTime() - g_flip_time
< 0x1c);` — a 28 ms floor, 35.7 Hz. The loop is therefore **at its ceiling** and
nothing in the shim is stalling it. No change was needed to `timeGetTime`'s
pacing, `WaitMessage`, or the flip spin; PORT-B's ASYNCIFY design holds up
under the real game exactly as it did under `legoland_shimtest`.

## 4. Blockers found on the way, with owners

| # | what | where | owner |
| --- | --- | --- | --- |
| B1 | `CreateThread` refuses, so `MusicThread` never runs and the music-on command line hangs in `RunGame` | `portable/src/hostwin/kernel32.c:395` | **PORT-A** |
| B2 | `_findclose(-1)` traps the module | `portable/src/hostwin/msvcrt.c:204` | **PORT-A** |
| B3 | 41 unwritten GAME functions in the closure; two are on the front-end path | `gen-browser/stubs.c` | **matching / PORT-M** |
| B4 | AVIFIL32 is stubbed and `PlayMovie` calls it even under `-nointro` | `gen_link` host stubs | PORT-B (a real AVI decoder is out of scope) |
| B5 | one trap ends the run; there is no "log and continue" mode | `portable/tools/gen_link.py` | **PORT-A / integrator** |

### B2 — `_findclose(-1)`, one line

```c
/* profiles.c 0x00491360 Goto_ProfileDir -- "Note the _findclose on a failed
 * handle — as shipped." */
h = _findfirst(g_str_profiles, &fd);
if (h != -1) { do { ... } while (_findnext(h, &fd) != -1); }
_findclose(h);                            /* UNCONDITIONAL */
```

```c
/* msvcrt.c:204 */
int _findclose(long handle)
{
    struct ll_find* f = (struct ll_find*)(intptr_t)handle;
    if (!f) return -1;                    /* -1 is not NULL */
    closedir(f->dir);                     /* reads address 0xffffffff */
```

With no `profiles` directory the handle is -1 and the module dies with
`RuntimeError: memory access out of bounds` inside `LoadProfilesFormDisk`
called from `GameFrame` — i.e. on the first front-end frame. The fix is
`if (handle == -1 || !f) return -1;`.

This lane's `main.c` creates `/gamedata/profiles` before `WinMain`, which the
host owes the game anyway (MEMFS starts empty and every profile the player makes
is written into it), and which incidentally routes the page around the bug. The
bug is still there for any other `_findfirst` that misses.

### B3 — the 41 unwritten GAME functions

`gen_link.py` emits a trapping stub for 41 names no source defines. Two are on
the front-end path and are hit within a second of `EnterFrontEnd`:

```
TRAP GAME ODFError      from llidb_odf.c    (0x0048...; called 3x in ObjDefFromFile)
TRAP GAME ObjDefFinalize from llidb_odf.c
TRAP GAME lrintf        from bnvpath.c, coaster10.c, coaster12.c...
```

`ODFError` is the one that stops a shipped-behaviour run, and it is worth being
precise about *why*, because it looks alarming and is not: it is a **diagnostic
printf on ordinary data**. llidb_odf.c:191/203/226 call it whenever an object
class's ODF record has an empty sprite / icon / build-anim name, which many
classes legitimately do — `obj->f68 = LoadSprite("InstituteIcon.lls", 4)` right
after is the fallback the game itself supplies. So the game is not in an error
state; it is logging. Its body wants to be the DBPrintf-shaped logger the other
`*Error` helpers are.

The full list, from `grep ll_gen_trap.*GAME gen-browser/stubs.c`:

```
ApplyMoodEvent rides.c            BoatingSchool_AddTake_I ridecb6.c
ClearObjectMenuIcons screens3.c   ClipPolygonPlanes unref1.c
Coaster3D_DrawLine unref3.c       Curve_InitCorner coaster7.c
DrawWrappedText unref4.c          EndScript screens3.c
FindFirstRider rides.c            FindNextRider rides.c
GetNthRider rides.c               LFPath_StepBloke lfentrance.c
LFPiece_GetFootprint logflume.c   ODFError llidb_odf.c
ObjDefFinalize llidb_odf.c        PhysVec_DerivativeTable unref1.c
Piece_InitCurve coaster3d.c       PlayTitleMovie screens3.c
ResumeGameTimer screens3.c        Romberg_Build coastershade2.c
SavedGame_48d470 bigscreens.c     SelectProfileSlot screens3.c
SetVidAnim screens3.c             ShowHelpString screens3.c
Span_EvalRange coaster11.c        StandardRemoveObject_B screencb.c
StandardRemoveObject_W screencb.c TrackGeom_BuildRamp coaster13.c
WindowProc screen.c               _errno_location savegame.c
lrint coaster3d.c                 lrintf (many)
sub_459820 sub_482a80 sub_48e3d0 sub_498cf0  screens3.c
```

**`WindowProc` (screen.c) deserves separate attention.** It is the window
procedure `RegisterClassExA` registers, so every message `DispatchMessageA`
delivers lands on a trap. Eight of the remaining names are in `screens3.c`, the
front-end screens file, and `SelectProfileSlot` is by its name the handler for
the very screen this lane reached. **Whoever takes the front end next should
take `screens3.c` and `screen.c`'s `WindowProc` as one piece of work.**

### B5 — no "continue" mode, and what this lane did about it

`ll_gen_trap` is `fprintf(stderr, "TRAP ..."); exit(70);`. That is right for
"where does the spine stop", and wrong for "what is on the path", because
`ODFError` — a logging call on ordinary data — hides everything behind it.

For this lane's own runs the GENERATED `gen-browser/stubs.c` in the build
directory (an untracked build artifact; nothing in the repo) was patched to
print once per distinct symbol and return when `LL_TRAP_CONTINUE=1`. Nothing of
that is committed, and every result reported here says which mode produced it.
The scratch patcher is `port-b4-trapcont.py`.

**Recommendation for `gen_link.py` (PORT-A / integrator):** make that an option
in the generator itself. It costs ten lines, it is opt-in through the
environment, and it is the difference between learning one blocker per build and
learning all of them in one run. This lane found nine in a single run that way.

## 5. Input: delivered by the host, ignored by the game

This is the open question the next lane inherits, so here is the measurement
rather than a conclusion.

**What the shim does, measured.** `dinput.c` now traces the mouse
`GetDeviceState` calls that carry something. Holding the left button down for 30
presented frames produces **exactly 30 trace lines** — one per frame — each
reporting the button set:

```
HOST DINPUT mouse state dx=0 dy=0 dz=0 buttons=100      (x30)
```

and a pointer move of (+199, +200) canvas pixels arrives once, whole:

```
HOST DINPUT mouse state dx=199 dy=200 dz=0 buttons=000
```

So `ProcessSystemEvents` → `ScanMouse` (input.c 0x00473a80) → `GetDeviceState`
runs once per frame and the game receives correct relative motion and correct
button state. The browser event queue drains to empty, and the DOM events are
the real ones the page's own listeners produce.

**What the game does.** Nothing. The presented frame is **byte-identical over
3,700 frames** (canvas checksum constant), the pointer stays stamped at its
origin, and none of Escape / Return / Space / Tab / ArrowDown / a printable key
/ a click changes a pixel.

**Ruled out, each checked:**

* *The clamp in `UpdateControllerFromMouseData` pinning x,y to 0.* It clamps
  against `g_screen->w/h` at `0x004bcbf4` → the record at `0x004bcbb0`, whose
  **image bytes are w=640 h=480** (read out of `original/legoland.exe` through
  `gen_link.load_image`). Not zero, so not the cause.
* *`g_game->in_game` being 0*, which would skip
  `g_input.point.x = g_controller->x` in `ReadGameButtons` (bighelp.c
  0x00452460). `in_game` is `+0x1e` of that same record and `InitSession`
  (startup.c:151) sets it to 1 as its first statement — and it is the SAME field
  `PresentFlip` calls `g_screencfg->cursor` and tests before stamping the
  pointer. The pointer *is* stamped, so the field is non-zero.
* *A layout disagreement about `Controller`.* input.c:57 and bighelp.c:12 give
  identical offsets (x +0x08, y +0x0c, dx +0x10, buttons +0x18), and all seven
  files that declare `g_controller` declare it as `Controller*` at `0x00813b00`.
* *A second `GetDeviceState` caller stealing the deltas.* There is only one
  (input.c:125).
* *An unported asm body silently doing nothing.* `LL_UNPORTED_ASM()` calls
  `abort()`; the page runs for minutes, so none of the 15 is on this path.
* *The presenter showing a stale buffer.* `ll_surf_Flip` and the primary-`Blt`
  path both present `g_ll_primary->bits`, which is the surface the game locks
  and draws into; and the same byte-identical frame appears in both the
  `WINDEBUG` (Blt) and full-screen (Flip) paths.

**What is left**, and where the next lane should start: the chain
`ScanMouse` → `UpdateControllerFromMouseData(g_controller)` (input.c:328, the
two calls are adjacent in one function) → `ReadGameButtons` (gameframe.c:512) →
`g_input.point`. Everything before it is proven; everything after it is
unobserved. The most likely remaining shapes are a live global that resolves to
two different objects — **exactly the class of defect PORT-A3 found with
`g_key_state` and `g_gpu_state`, and `g_key_state` is `ScanKeyboard`'s
destination** — or a mis-wired call the way PORT-M2's `NewScriptEvent` /
`AddScriptString` was. `g_mouse_state` (input.c) and `g_controller` are the two
objects to check first with `linkreport.py` / the gen-browser manifest.

Note the shim can be re-measured at any time: `?trace=1` plus the `DINPUT mouse
state` lines say precisely what the game was handed, and they are cheap (a few
lines per gesture, not one per frame).

## 6. Reproducing

```bash
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$HOME/.venvs/legoland/bin/python
ninja -C portable/build-wasm legoland_browser
cd portable/build-wasm && python3 -m http.server 8795
# http://localhost:8795/legoland.html?args=-nointro+-nomusic+WINDEBUG&trace=1
```

As shipped the page reaches `TRAP GAME ODFError`. To see the front end, apply
the untracked trap-continue patch to `gen-browser/stubs.c` (§B5) and set
`ENV.LL_TRAP_CONTINUE='1'` in the page's `preRun`.

Headless, the same two runs:

```bash
LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main \
  node portable/build-wasm/legoland_headless.js                 # -nomusic: reaches ODFError
LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main \
  node portable/build-wasm/legoland_headless.js -nointro WINDEBUG  # music on: hangs (B1)
```

Note `LL_DATA_DIR` must be **absolute** — `name_trap.py` passes it through and a
relative path fails the chdir with a bare message.

## 7. Screens reached, and with what

| screen | how it was reached | what it needed |
| --- | --- | --- |
| title screen | automatic (`ShowTitleScreen`, before the music wait) | PORT-B3's RLE painters |
| **`PLAYER DETAILS`** (front-end profile screen, 8 slots) | **automatic** — `RunGame` leaves the music wait, loads the interface, `EnterFrontEnd`, and `GameFrame` runs `InitScreens(0)` → `InitListProfiles` | the succeeding `IDirectSound` (§1); `/gamedata/profiles` existing (§B2); traps continuing past `ODFError` (§B5) |

No screen was reached *by* input, because no input has any effect (§5). None was
needed either, and that is a fact about the run rather than a reading of the
code: `RunGame` calls `ShowTitleScreen` and then, once the music wait clears,
walks through `LoadIconBarGFX` … `EnterFrontEnd` and into `GameFrame` without
ever asking for a click. The brief expected a click to leave the title screen;
there is none to give.
