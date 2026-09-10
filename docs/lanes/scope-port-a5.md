# Scope PORT-A5 — the trap-continue mode, the music thread, the CRT's -1, and THE INPUT QUESTION

> **Status: IN PROGRESS (claimed 2026-09-12 by PORT-A5).** Branch
> `scope/PORT-A5` from `feat/decomp-completion-next-steps-24a0d6` @ `0e0b2b76`
> (the PORT-B4 merge). Files owned: `portable/tools/gen_link.py`,
> `name_trap.py`, `portable/cmake/headless.cmake`, `portable/src/headless/**`,
> `portable/src/hostwin/kernel32.c`, `msvcrt.c`. `LEGOLAND/*.c` untouched, so
> the VC6 gate has nothing to check for this lane.

**Headline: the front end can be driven.** PORT-B4 left three blockers and one
open question; all four are closed. The question — input arrives at the shim and
the front end ignores it — was a generator defect of exactly the class PORT-A3
found, and the biggest instance of it in the image: the 164-byte `GameInput`
record the front end reads every frame was emitted as **nine separate objects**,
so the files that write it and the files that read it addressed different memory.

| # | what | where it was | state |
| --- | --- | --- | --- |
| B1 | `CreateThread` refuses, music-ON hangs in `RunGame` | kernel32.c | **closed** (§2) |
| B2 | `_findclose(-1)` kills the module on the first front-end frame | msvcrt.c | **closed** (§3) |
| B5 | one trap ends the run; no "log and continue" | gen_link.py | **closed** (§1) |
| §5 | input reaches the shim, the front end ignores it | gen_link.py | **closed** (§4) |

Gates: wasm `ctest` **11/11** (was 10; `probe_input` is new), native `ctest`
**5/5**, `legoland_linkcheck` links on both, every wasm target builds
(`legoland_headless`, `_debug`, `legoland_tests`, `pathtest`, `shimtest`,
`browser`). `headless_spine`'s title-screen checksum `0x4a092b01` is unchanged,
which is the evidence that the three merged records did not move a pixel of what
already worked.

---

## 1. `LL_TRAP_CONTINUE=1`: every blocker in one run

`gen_link.py`'s `ll_gen_trap` printed one line and `exit(70)`. That is right for
"where does the spine stop" and wrong for "what is on the path", because a
generated trap is usually a **logging call on ordinary data** (`ODFError`, which
llidb_odf.c calls whenever an object class has an empty sprite name — many
legitimately do) or a subsystem the port does not have (AVIFIL32). Stopping at
the first hides every other. PORT-B4 found nine by patching the GENERATED
`stubs.c` by hand every build; this is the same thing as an option in the
generator.

`LL_TRAP_CONTINUE=1` in the environment makes the helper print each distinct
symbol once and **return**. Read once (a trap can be inside the frame loop);
de-duplicated by the `name` POINTER first and `strcmp` second, so one literal is
one symbol whether or not the linker merged literals; default unchanged.

`name_trap.py --continue` sets it and then lists every trap in the order the game
reached them, **and still runs the stack analysis**, because in that mode a trap
is not what killed the run. Exit code 1 when any trap was hit.

**A returning trap hands its caller a zero it never computed.** The mode makes a
list of work and never the claim that something works; the docstring, the
`--continue` help and the generated comment all say so, and every result below
says which mode produced it.

### The list, post-fix, one run (`LL_TRAP_CONTINUE=1 legoland_headless`)

Nine, unchanged from PORT-B4's count, which is itself the news: fixing B1/B2 and
the input record added no new blocker.

```
TRAP GAME      ODFError             from llidb_odf.c     <- a DBPrintf-shaped logger
TRAP GAME      ObjDefFinalize       from llidb_odf.c
TRAP GAME      lrintf               from bnvpath.c, coaster10.c, coaster12.c...
TRAP AVIFIL32  AVIFileInit          from advisor.c, movie.c
TRAP AVIFIL32  AVIFileOpenA         from advisor.c, movie.c
TRAP AVIFIL32  AVIFileInfoA         from advisor.c, movie.c
TRAP AVIFIL32  AVIFileRelease       from advisor.c, movie.c
TRAP AVIFIL32  AVIFileExit          from advisor.c, movie.c
TRAP AVIFIL32  AVIStreamGetFrameOpen from advisor.c, movie.c
```

Three of those are PORT-M4's (the externs with no address comment) and six are
PORT-B's AVI stubs. **The run then stays up**: `\ncannot open output file` x7 is
`LoadProfilesFormDisk` walking `profiles\Profile1..7.txt` and finding none, which
is the shipped behaviour of a fresh install and proves that function now
completes — before §3 it faulted inside it.

### Also in `name_trap.py`

* `--cd-dir` / `--data-dir` go through `os.path.abspath` and are checked to be
  directories. The harness `chdir()`s to `LL_DATA_DIR` before `WinMain`, so a
  relative path failed with a bare message **and** `LL_CD_DIR` was then resolved
  against the new cwd and silently named nothing, which looks like a missing CD.
* `TRAP_RE`'s dll field is non-greedy, so `TRAP GAME 0x00481234 Foo` names `Foo`
  and not the address (an unwritten body WITH a known address prints two tokens
  there).

## 2. `CreateThread`: the decision, and why

**`g_music_disabled` is not "music is off".** MusicThread writes it on every
failure exit AND on its success path — musicthread.c:678, one line before
`g_music_ready = 1` and the message loop. It means *"the music thread has
finished starting up, stop waiting for it"*. `RunGame`'s
`while (g_music_disabled == 0) { PeekMessageA; Sleep(100); }` (gamemain.c:370)
therefore waits for the thread, and with `CreateThread` refusing — while
`InitMusicSystem` reports success on a null handle anyway, sysstubs.c:394 —
nothing ever wrote it. Only the thread knows; so the host has to run the thread.

**DECISION: run the start routine inline, to completion, on the main stack, with
a `setjmp`/`longjmp` escape from a wait that can never be satisfied.** That is
the brief's option (a), and it reaches option (c) without the host having to know
which rung the thread will take:

* ole32's `CoCreateInstance` reports `REGDB_E_CLASSNOTREG` by design (PORT-B4's
  dsound.c), so MusicThread takes its first `shutdown:` rung at musicthread.c:565
  — `g_music_ready = 0; g_music_disabled = 1; return 0` — after 273 of its 3,161
  instructions. It never creates its events, builds a segment, or reaches the
  message loop.
* **Option (b), a per-frame pump through `ll_host_pump_timers`, cannot host this
  routine at all.** `MusicThread` is one function with an infinite message loop,
  not a step function: calling it once per frame would restart its COM bring-up
  every frame. Hosting it that way needs a real coroutine (an ASYNCIFY fiber),
  and there is nothing to host until DirectMusic exists.
* Faking the flag from the host is out of bounds twice over: it is game state at
  0x007988bc, and `g_music_ready`, `SuspendMusicThread`, `ResumeMusicThread` and
  `KillMusicSystem` would all then be operating on a thread that never existed.

`CreateThread` returns a real `LL_H_THREAD` handle (so `KillMusicSystem`'s
`TerminateThread` and resaudio2.c's Suspend/Resume have something to hold), fills
`*id`, refuses a NESTED inline thread (one `jmp_buf`; nothing nests), and reports
an ignored `CREATE_SUSPENDED` rather than half-implementing it. `setjmp`/`longjmp`
were verified on this toolchain plain and under `-sASYNCIFY` before any of this
was written.

Measured, `legoland_headless -nointro WINDEBUG` — **no `-nomusic`**:

```
HOST DirectSoundCreate -> 0xc61898 (silent device, simulated cursor)
HOST CreateThread: running the start routine inline
HOST CoInitialize: no COM runtime; the result is discarded
HOST CoCreateInstance: REGDB_E_CLASSNOTREG (no DirectMusic)
HOST CreateThread: the routine ran to completion, returning 0
...                                          (and on to the front end)
```

### The wait side had to cooperate, and the bug there was live

`WaitForSingleObject` answered `WAIT_OBJECT_0` for every handle. But
`WaitForSingleObject(event, 0)` is a **poll**, and MusicThread's message loop is
built out of two of them (musicthread.c:686/722) — answering "posted" to both
makes the thread act on a command nobody sent, out of a `g_imt_cmd` nobody wrote.
Now:

| handle | wait | answer |
| --- | --- | --- |
| mutex, thread | any | `WAIT_OBJECT_0` — nothing else can hold it (startup.c:239's one-instance check depends on this, and is the only game caller outside MusicThread) |
| event, signalled | any | `WAIT_OBJECT_0`, consumed if auto-reset |
| event, unsignalled | `ms == 0` | `WAIT_TIMEOUT` |
| event, unsignalled | `ms != 0` | unsatisfiable: inside an inline thread the routine is unwound; on the main thread reported once and `WAIT_TIMEOUT` (no game code does it) |

`WaitForMultipleObjects` returns the index of the posted event and treats
"every handle is an unposted event" the same way.

### When this stops being enough

If DirectMusic ever becomes real, MusicThread reaches
`WaitForMultipleObjects(2, ev, 0, INFINITE)` (musicthread.c:685) with both events
unposted. Inline, that is a deadlock, the escape fires, the trace says so in one
line, and the music set is built and downloaded but no command the game posts
afterwards is ever handled. **That** is the point at which the loop needs a
fiber. It cannot happen silently.

## 3. The CRT's failure value is -1, which is not NULL

profiles.c's `Goto_ProfileDir` (0x00491360) calls `_findclose(h)`
**unconditionally**, as shipped — on Windows a documented -1/EINVAL return. Here
the handle is a `struct ll_find*` cast to `long`, and `if (!f)` does not catch
-1: `(struct ll_find*)(intptr_t)-1` is a perfectly good pointer to 0xffffffff. So
with no `profiles` directory the module died with `memory access out of bounds`
inside `LoadProfilesFormDisk` on the **first front-end frame**. The browser page
escaped it only because PORT-B4's `main.c` creates `/gamedata/profiles`; node hit
it every run, and it is what stopped this lane's first trap-continue run.

`ll_bad_find` is the -1-or-0 test in one place, and all three entry points use
it. The audit of the rest of the file, which is also a comment in it:

| family | failure value | state |
| --- | --- | --- |
| fd (`_open` `_close` `_read` `_write` `_lseek` `_tell` `_filelength`) | -1 | already safe — -1 goes to POSIX, which answers EBADF. audio4.c:152/227 `_close` on a failed `_open` behaves as it does on Windows |
| find (`_findfirst` `_findnext` `_findclose`) | -1, and the handle is ours | **was broken in all three**, fixed; `_findfirst` also rejects a null spec/out buffer, `ll_find_step` checks `f`/`f->dir`/`out` |
| pointer (`fopen` `_getcwd` `_strupr` `_msize`) | NULL | `fopen` fine; `_msize(NULL)` and `_strupr(NULL)` were unguarded dereferences where MSVC answers, guarded; `_splitpath(NULL)` empties its out buffers |

`_stat`/`_fstat`/`_access`/`_unlink`/`_chmod` are not defined here and a grep of
the whole CRT surface across `LEGOLAND/*.c` finds no caller — the only names
beyond the families above are `_filelength` (2), `_getcwd` (4), `_msize` (4),
`_splitpath` (15), `_strupr` (3), `_tell` (7).

## 4. THE INPUT QUESTION: the record was nine objects

### The finding

PORT-A3's interior-alias pass takes an object's extent from the game's own
declaration — array bounds times an element size that is a language fact. `extern
GameInput g_input;` gives **nothing**: a struct type has no size until somebody
writes the struct out, and gen_link does not parse C. So the record fell back to
the gap tiling, which sizes an object by the distance to the next NAMED address —
and for a record whose fields other files declare individually, **that next
address is its own second field**.

164 bytes of `GameInput` at 0x00813a40 came out as nine separately 16-byte-aligned
blocks, and the two halves of the game disagreed about where every field is:

```
bighelp.c  ReadGameButtons (0x00452460):  g_input.point.x = g_controller->x
                                          -> g_input + 4, i.e. the PADDING after
                                             a four-byte block
screens3.c (the front-end screens):       g_gfx_point     (0x00813a44 = +0x04)
                                          g_mouse_buttons (0x00813ac4 = +0x84,
                                             which is g_input.btn0.state)
                                          -> two other objects entirely
```

The cursor point alone is read under four names in 22 files — `g_gfx_point` (17),
`g_mouse_point` (4), `g_input_point` (3), `g_mouse`, `g_raster_origin` — against
six files that write it as `g_input.point`. Both halves are live every frame. That is PORT-B4 §5's measurement exactly: the host delivers, `ScanMouse`
receives, the Controller moves, `ReadGameButtons` runs — and nothing the front end
reads ever changes.

### The evidence: `legoland_headless --probe-input`, the same probe on both closures

```
pre-fix (STRUCT_EXTENTS emptied, one rebuild)        post-fix
  g_gfx_point        +0x10    WRONG (+0x4)            +0x4     OK
  g_fp_w          +0xea580    WRONG (+0x2c)           +0x2c    OK
  g_mouse_buttons    +0x40    WRONG (+0x84)           +0x84    OK
  g_mouse_btn_a   +0xea5f0    WRONG (+0x94)           +0x94    OK

  ReadGameButtons wrote (440,300); READERS read (5,4)   READERS read (440,300)
  g_mouse_buttons 0x00                                  g_mouse_buttons 0x05
```

`g_fp_w` was **958,336 bytes** away from the field the writers write.

The probe injects one gesture through the shim's own entry points
(`ll_host_mouse_move(+120,+60)`, `ll_host_mouse_button(0,1)`,
`ll_host_key_set(DIK_ESCAPE,1)`) and prints every link of the chain:

```
after Scan*              key[ESC]=80  mouse=(120,60,0) btn=800000
                         controller x=320 y=240 buttons=0x000
after UpdateController    controller x=440 y=300 dx=120 dy=60 buttons=0x201
after ReadGameButtons    g_input.point=(440,300) btn0.state=0x5
                         | READERS: g_gfx_point=(440,300) g_mouse_buttons=0x05
INPUT OK
```

(`buttons=0x201` is mouse button 0 | DIK_ESCAPE, per input.c:70's bit map.) It
opens with the layout gate, which is pure address arithmetic over the record's
names — no input, no game state, and it is what actually found the bug.
Registered as the ctest **`probe_input`**.

### The fix, and the two other records the sweep found

`gen_link.py` grows `STRUCT_EXTENTS`, keyed by address. A row is only allowed
when the game's own sources pin the extent **twice** — the struct's last field
offset, and an independent `extern` at that field's own address whose comment
gives the same number — and both citations are in the row. The sweep that found
the candidates is every extern whose address comment attributes it to a record:

```
grep -hoE "/\* 0x00[0-9a-f]{6} +[a-zA-Z_]\w*\.[\w.]+" LEGOLAND/*.c | sort -u
```

Three records, all split, **all on the front-end path**:

| record | addr | was | now | interior names | what was broken |
| --- | --- | --- | --- | --- | --- |
| `GameInput` | 0x00813a40 | 4 | 176 | 12 | the cursor point, fp_w/fp_h and five GameButton pairs — input, every frame |
| `PopUpUI` | 0x007fdea4 | 4 | 380 | 72 | `g_popup_x`/`g_popup_y`, every `g_pu_icon_*`, `g_info_active`, the mock-object table — popups and the info panel |
| `Profile` | 0x007cad60 | 30 | 288 | 6 | the PLAYER DETAILS scratch profile: name, age, save type, the three volumes |

Citations, for review: `GameInput` bighelp.c:35-64 (`move_tick` +0xa0) with
bubblecache.c:120 `g_input.fp_h` +0x30; `PopUpUI` popup.c:474-507 (`spr_happy`
+0x174, whose own comment reads 0x007fe018 = 0x007fdea4 + 0x174) with misc3.c:216
`g_popup.icon_next` +0x120; `Profile` profiles.c:64-77 (0x110, `f10f` +0x10f) with
unref7.c:276 `g_temp_profile.age` +0x20. The generated offsets came out equal to
the documented ones for every one of the 90 new aliases, including the last field
of each record — which is the independent check that the extents are right.

The merge still goes through A3's clamp (rule 2: stop at any address a game
object defines), so a row can only claim storage that was going to be tiled to
these names anyway. All three records are **all-zero in the image**, so no
pointer word is involved and globals.c's bytes do not change.

### Census: 57 fewer definitions, the same bytes

`gen/manifest.md`, wasm32 `-DLL_ILP32=ON`, before → after:

```
- globals defined: 2149 -> 2092   (3705508 bytes BOTH times)
- data aliases: 367 -> 334
- objects merged from interior-aliased names: 15 -> 18 (100 -> 190 offset aliases)
- words re-pointed at a symbol address / INTO a block: unchanged
- host API stubs: unchanged
```

Link report, `portable/build-wasm` (258 objects, 6540 defined, 46 undefined):
game-fn 0, game-data 0, alias 0, host 0, crt 43, unknown 3
(`_errno_location`, `_indirect_function_table`, `_stack_pointer`), duplicates 0,
asm stubs 15, prototype conflicts 420. Unchanged by this lane: nothing here adds
or removes a symbol, it only changes how many objects the same bytes live in.

### What the integrator and the next lane must know

1. **The optimizer hazard A3 documented grows with this change.** An interior
   alias is invisible to the compiler: two names in one TU whose extents OVERLAP
   may be assumed not to alias, so a store through one and a load through the
   other IN THE SAME FUNCTION can read a stale value. A3 measured 45 such pairs
   tree-wide (42 pre-existing). These three records add more, and one is worth
   naming: **`gameframe.c` declares `GameInput g_input` (line 788) and `Pos
   g_gfx_point` (line 264)**, and `popup.c` declares `PopUpUI g_popup` (508) and
   `GameInput g_input` (518). Nothing measured misbehaves — the front end runs
   and `probe_input` passes at the browser target's `-O2` — but if a front-end
   screen ever reads a cursor position one statement after writing it and sees
   the old one, this is the mechanism, and `volatile` on the lvalue is the fix
   (as in `portable/tests/test_keystate.c`).
2. **The sweep is not exhaustive.** It finds records a source documented with a
   `g_owner.field` comment. A record whose fields were all recovered under
   separate names, with no comment tying them together, is still split and still
   silent. The honest general fix is for `declared_extent` to compute `sizeof`
   from the struct definition in the declaring TU; that is a small C parser and
   was out of this lane's time, so `STRUCT_EXTENTS` is the table that makes each
   one a one-line, twice-cited change until then. **A split record is the first
   thing to suspect for any "the game ignores X" report.**
3. `probe_input` and `headless_spine` both need `gamedata/`, so CI (which has no
   assets) still runs neither; the asset-free `ctest` set is unchanged.

## 5. Reproducing

```bash
PY=$HOME/.venvs/legoland/bin/python
emcmake cmake -S portable -B portable/build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
ninja -C portable/build-wasm legoland_headless
D="LL_CD_DIR=$PWD/gamedata/disc LL_DATA_DIR=$PWD/gamedata/main"    # both ABSOLUTE

# the input chain, end to end, in a second
env $D node portable/build-wasm/legoland_headless.js --probe-input

# music ON -- no -nomusic -- now leaves RunGame's wait
env $D node portable/build-wasm/legoland_headless.js -nointro WINDEBUG

# every blocker on the path, one run
env $D LL_TRAP_CONTINUE=1 node portable/build-wasm/legoland_headless.js
$PY portable/tools/name_trap.py --continue

(cd portable/build-wasm && ctest)        # 11/11
```

The browser page is unchanged and picks all of this up from the closure:
`?args=-nointro+WINDEBUG&trace=1` no longer needs `-nomusic`, and
`ENV.LL_TRAP_CONTINUE='1'` in `preRun` is now a supported mode rather than a
patch to a build artifact.
