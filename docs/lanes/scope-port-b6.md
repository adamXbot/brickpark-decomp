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
