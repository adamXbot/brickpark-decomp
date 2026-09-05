# fable-c lane notes — `LEGOLAND/sysmisc2.c`

Five functions from three subsystems (playable samples, the interactive-music
theme mailbox, CD-volume probing), 412 instructions. **All five exact**;
`audit.py` PASS, `/W3` clean. Written 2026-09-05 alongside the integrating
session, as the direct companion to scope B's `sysmisc.c`.

| address | function | insns | audit | note |
| --- | --- | --- | --- | --- |
| 0x004928a0 | `StartPlayableSample` | 48 | **[OK]** | first compile |
| 0x00492ce0 | `SetTheme` | 50 | **[OK]** | first compile |
| 0x004965a0 | `SetSampleScreenPos` | 84 | **[OK]** | 2 levers, ~20 variants |
| 0x004510e0 | `RES_FindVolumeOnResPath` | 92 | **[OK]** | 3 levers, ~15 variants |
| 0x00450f30 | `RES_FindVolumeOnAnyDrive` | 138 | **[OK]** | 3 levers, ~85 variants |

Markers committed (all `// FUNCTION:` on the line immediately above the
signature, no parenthetical):

```
// FUNCTION: LEGOLAND 0x004928a0
// FUNCTION: LEGOLAND 0x00492ce0
// FUNCTION: LEGOLAND 0x004965a0
// FUNCTION: LEGOLAND 0x004510e0
// FUNCTION: LEGOLAND 0x00450f30
```

---

## Levers, with evidence

- **A coordinate pair that feeds a SUM OF SQUARES must be ONE `Pos` aggregate,
  not two `int` locals — the aggregate member becomes the destination symbol of
  its own product.** `SetSampleScreenPos` ends in
  `mov edx,edi / mov eax,esi / imul edx,edi / imul eax,esi / add edx,eax`, i.e.
  `y*y` in edx and `x*x` in eax. As two plain `int` locals VC6 emits the mirror
  (`x*x` in edx) and it is **unreachable by every spelling**: both operand
  orders, four named-temporary shapes (`dy2`/`dx2` singly and together, `d2 =
  …; d2 += …` both ways), an inlined `Sq(int)` and an inlined `DistSq(int,int)`
  called both ways — all byte-identical at 80 of 84 — and six free volatile
  reads, which either do nothing (4 of 84) or fix the registers at the price of
  an extra instruction (2 of 85). Declaring the pair as one `Pos p` and using
  `p.x`/`p.y` throughout is **84/84, and both sum orders then work**. This is
  the same mechanism DECOMP records for a by-value `Pos` parameter
  (`Restaurant1_WalkToSeatSpot`) and for `WalkPath_Advance`'s aggregate
  placement, now measured on a **local** aggregate at an arithmetic site.

- **A call result handed straight to a COM method must be a named local.**
  `s->buf->lpVtbl->SetVolume(s->buf, VolumeFromDistSq(d2))` makes VC6 evaluate
  the object and its vtable pointer BEFORE the inner call, spill the vtable
  pointer to the frame and take a fourth callee-saved register: 90 instructions
  for an 84-instruction function. `vol = VolumeFromDistSq(d2); … SetVolume(…,
  vol);` puts the inner call first, and the original's
  `call / mov esi,[esp+0x14] / add esp,4 / mov ecx,[esi+0x2c] / push eax /
  push ecx` falls out. Same family as DECOMP's "two struct-return handles must
  be separate statements", from the *callee-object* side rather than the
  argument side.

- **`field >> 1` on an `unsigned short` is `shr`; `-field >> 1` is `neg` +
  `sar`.** After the `xor eax,eax / mov ax,[mem]` zero-extending load VC6 knows
  the value is non-negative and shifts it unsigned; the unary minus widens it to
  a signed int and the same `>> 1` becomes arithmetic. Both appear on the SAME
  field within four instructions in `SetSampleScreenPos`, so the pair is the
  read-side tell for a `u16` field used in signed arithmetic. (It also means the
  original's two clamp thresholds round in opposite directions for an odd
  viewport width — behaviour, not a bug, since the viewport is 640×480.)

- **`char root[4] = "c:\\";` is a `mov eax,[literal] / mov [esp+N],eax` pair,
  not an immediate store.** A four-byte string-literal array initialiser is
  copied out of `.rdata`; `strcpy(root, kLit)` with `#pragma intrinsic(strcpy)`
  is the `repne scasb` + `rep movsd` expansion instead, sixteen instructions
  dearer on the first `RES_FindVolumeOnResPath` draft.
  `memcpy(root, kLit, 4)`, `*(RootPath*)root = kRoot` (a 4-byte struct) and
  `*(unsigned long*)root = *(const unsigned long*)kLit` are **all byte-identical
  to the initialiser** — the initialiser is simply the plausible source.

- **Three `return 0`s do NOT produce the zero-register join; a flag set in the
  innermost block does.** `RES_FindVolumeOnResPath`'s failure block is
  `mov eax,edi` with `xor edi,edi` eleven instructions earlier — edi is pushed
  for nothing else. Three textual `return 0;` guards tail-duplicate the
  six-instruction epilogue three times (115 instructions for a 92-instruction
  function); `goto fail; … fail: return 0;` merges them but emits `xor eax,eax`
  in the merged block and drops the callee-saved push (88, four short). What is
  exact is `int found = 0;` with the three tests written as **nested ifs** whose
  innermost body is `found = 1;`, and `return found;` at the end — or the same
  thing as `goto done` with a trailing `found = 1; done: return found;`. Both are
  92/92. Sharpens DECOMP's "`xor <callee-saved>,<same> / mov eax,<it>` proves a
  two-predecessor JOIN": the join here has THREE predecessors and the zero must
  be a *variable* that is live from before the first call, which is what forces
  it into a callee-saved register.

- **`found = 1;` BEFORE the `strcpy`, not after — worth 18 of 138.** In
  `RES_FindVolumeOnAnyDrive` the flag store lands in the middle of the intrinsic
  `strcpy`'s expansion (`mov edi,OFFSET / mov ebp,1 / shr ecx,2 / rep movsd`).
  Written after the copy (the natural order, and the order that reads correctly)
  VC6 exiles `found` to a **stack home**, the frame grows from 0x218 to 0x21c,
  and the incoming `vol` parameter takes ebp instead — 117 to 120 of 140,
  depending on the loop-increment form. Written first, `found` keeps ebp, `vol`
  is reloaded from its argument slot every iteration exactly as the original
  does, and the body is 135 of 138 (138/138 with the increment lever). **The
  statement order decides which of two webs is enregistered**, and the loser is
  a parameter whose home is free — so this is invisible in the instruction
  stream except as the frame size.

- **The mask guard must be `if (mask) { loop }`, not `if (!mask) return
  found;`, and that decision GATES the one above.** Both forms produce the
  original's two distinct epilogue blocks (the early one popping two registers,
  the late one four), so the layout does not distinguish them — but with the
  early return VC6 coalesces `found = 0` with the loop counter's `i = 0` into
  one hoisted `xor ebx,ebx` at entry, stores the flag straight back out to a
  stack slot (`sub esp,0x21c`) and never gives it a register again. The
  `found = 1`-first lever is then completely inert: the early-return form is
  110 of 139 with the store in either position. Inside `if (mask)` the flag is
  enregistrable and the store order decides which web wins. The three levers
  reach 138/138 only **together** — 117, 120 and 135 of 138 for the partial
  combinations.

- **`bit <<= 1` belongs in the `for`'s INCREMENT clause.** The original's loop
  tail is `mov ecx,[bit] / inc ebx / shl ecx,1 / cmp ebx,0x20 / mov [bit],ecx /
  jl` — the counter's `inc` is interleaved between the shifted value's load and
  its shift. In the loop BODY the load/shift/store come out as a block and
  `inc ebx` follows (135 of 138). In the increment clause as
  `for (i = 0; i < 32; i++, bit <<= 1)` it is 138/138; as
  `for (i = 0; i < 32; bit <<= 1, i++)` it is 135 again, and so are a `do/while`
  and a hand-written `i++` at the end of the body. Only
  `{ i++; bit <<= 1; }` written in that order at the end of the body also
  reaches 138. Confirms and sharpens DECOMP's "a decrement in the
  for-increment vs the body reorders two ALU ops": **the counter's update must
  be generated BEFORE the other induction variable's**, whichever clause holds
  them.

- **Negative, recorded so it is not re-derived (about 60 compiles):** nothing
  else moves `RES_FindVolumeOnAnyDrive`'s `found`/`vol` allocation. Measured and
  inert: `found` declared first / last / as a statement after the guard; the
  guard returning `found` vs `0`; `!mask` vs `mask == 0`; `bit & mask` vs
  `mask & bit`; `register int found`; `for` / `while` / `do-while`; `unsigned
  long` vs `int` vs `unsigned int` for mask and bit; `(char)(i+'A')` /
  `(char)i + 'A'` / `'A' + (char)i`; named locals for the `GetDriveTypeA` and
  `GetVolumeInformationA` results; an inlined `DbgLine()` wrapping the
  printf/flush pairs at one, two or three sites; an inlined `VolumeNameOk()`;
  `!vol ||` vs `vol == 0 ||` vs a `goto hit/miss` pair vs a duplicated success
  block (that one costs 14 instructions — VC6 does not cross-jump them); a
  block-scoped `bit`/`i`; and `mask & (1 << i)` instead of a walking bit (VC6
  does **not** strength-reduce a shift by an induction variable — it emits
  `mov edx,1 / mov ecx,ebx / shl edx,cl` inside the loop, so a `bit` variable
  in a stack slot is genuinely in the source). Free volatile reads on `vol`
  (three placements), on `bit`, and a `volatile unsigned long bit` all change
  the regime without closing it. **The two statement-order levers above are the
  whole difference**, which is why the sweep looked like a floor for 80
  compiles: `strict 39 / register-blind 24` read as "structural" and it was —
  the structure was two statement positions, not a construct.

---

## Mechanics recovered

- **`StartPlayableSample` (0x004928a0)** — the internal audio2.c/audio3.c call
  behind `PlaySample`. Guards on `g_samples_ready`, a non-null sample and a
  non-null `Sample::def` (i.e. the record must be a playable INSTANCE, not a
  definition), then `IDirectSoundBuffer::Play(buf, 0, 0, flags)` with
  `DSBPLAY_LOOPING` when `flags & 4` is set and 0 otherwise, and finally clears
  flag bit 1. Returns 1 only if `Play` succeeded. **`Sample::flags` bit 2
  (0x0004) is the LOOPING bit** — audio3.c's field comment did not name it.

- **The interactive-music ("IMT") command mailbox**, four globals, named here
  for the first time. A worker thread owns `g_imt_state` (0x004bf778) and is
  driven through `g_imt_cmd` (0x0079a6a4) + `g_imt_cmd_arg` (0x0079a6a8) and
  woken with `SetEvent(g_imt_event)` (0x0079a6a0). `g_imt_theme` (0x0079a6ac)
  is the theme currently playing. Opcodes seen: **1** = 0x00492d80 (a two-line
  poster, no argument), **3** = 0x00492ca0 `SetThemeInTransition` (theme % 5,
  only while state is 1 or 2), **4** = `SetTheme`'s own post. States: **1/2** a
  transition is being set up, **5/6** one is queued, **7** one is running.
  `SetTheme` (0x00492ce0) dispatches on state, logs through `DBPrintf` with the
  three `IMT:` strings at 0x004bf7cc / 0x004bf7a4 / 0x004bf77c, and the `%`
  really is signed (`cdq / idiv`), so the theme id is a signed int.

- **`SetSampleScreenPos` (0x004965a0)** — the positional-audio placement every
  arm of `UpdateSampleSource` tails into (scope B named it but did not open
  it). It reads the viewport size at **0x004bcbf4 +0x10/+0x12** (`view_w` /
  `view_h`, the same fields bigrender.c and fpui3.c use), centres the sample's
  screen position on the viewport, and then splits:
  - **pan** = `PanFromOffset(x)` (**0x00496570**, first named here) = the RAW
    centred x times 4, clamped to the DirectSound range ±10000;
  - **volume** = `VolumeFromDistSq(x*x + y*y)` (**0x00496540**, first named
    here) where x and y have first been folded to zero inside a band of half
    the viewport — so anything in the middle of the screen is at full volume
    and only the distance PAST the screen edge attenuates. The helper is
    `g_sfx_master_db (0x007988a0) - d2/60`, and it returns DSBVOLUME_MIN
    (-10000) if that is below -3000 **or above 0**.
  Both go into the buffer through `SetVolume` (+0x3c) and `SetPan` (+0x40);
  the function returns 1 only if both succeed.

- **The two CD probers, in full.** Both build a three-character root path,
  call `GetVolumeInformationA` with 256-byte volume-label and file-system-name
  buffers, and require the file system to be **`"CDFS"`** (0x004b85ec) —
  that is the actual "is this a CD" test, `GetDriveTypeA` only pre-filters.
  When the caller passes a volume name it must also match the label; a null
  name means "any CD".
  - **`RES_FindVolumeOnResPath` (0x004510e0)** probes exactly one drive, the
    one named by `g_res_path[0]` (0x00813b04), and has no logging.
  - **`RES_FindVolumeOnAnyDrive` (0x00450f30)** logs
    `"Checking all drives (Mask = %d)"`, walks all 32 bits of
    `GetLogicalDrives()`, and for each `DRIVE_CDROM` (5) logs
    `"Getting Info on drive %s"` before probing. On a hit it **writes the drive
    root back into `g_res_path`** and logs `"Drive %s contains the correct
    CD"`. That write is the coupling between the two probers: after any
    successful all-drives scan, the ResPath prober checks the right drive.

---

## Original bugs reproduced (commented at the site)

- **`RES_FindVolumeOnResPath` never upper-cases the drive letter.** The
  `toupper` call is there with the right argument (`movsx ecx,al / push ecx /
  call 0x0049f34b / add esp,4`) but its RESULT IS DISCARDED — the raw character
  was already stored into `root[0]` one statement earlier. The author wrote
  `toupper(root[0]);` where he meant `root[0] = toupper(root[0]);`.
  `GetVolumeInformationA` ignores case, so the bug never shows.
- **`RES_FindVolumeOnAnyDrive` never breaks out of the drive loop.** All 32
  mask bits are walked even after a match, so with two CD drives each holding a
  matching disc `g_res_path` is left naming the LAST one and the success line
  is logged twice.
- **`SetTheme` posts `theme % 5` on the fall-through path but the caller's RAW
  theme on the state-5/6 path.** A theme id of 5 or more queued while a
  transition is pending reaches the worker unwrapped, where every other path
  wraps it. Reproduced.

## Extern-type and naming divergences (do NOT "align" these)

- **`StartPlayableSample` takes `Sample*` here.** audio2.c declares it
  `int StartPlayableSample(PlayableSample* s)`, audio3.c
  `int StartPlayableSample(Sample* s)`. Both are correct as caller-side
  declarations; this file defines it over its own copy of audio3.c's `Sample`.
- **`SetTheme` is `void SetTheme(int theme)`** — matching input.c's extern. The
  `cdq / idiv` proves the parameter is a SIGNED int, so an `unsigned` spelling
  would be wrong here even though every caller passes 0..4.
- **`SetSampleScreenPos` returns `int`**, as sysmisc.c already declares it
  (`int SetSampleScreenPos(Sample*, int, int)`); `UpdateSampleSource` tails
  into it and returns its result.
- **0x004bcbf4 gains a sixth name.** legoland.h `Map* g_map`, screen.c
  `ScreenCfg* g_screencfg`, gpu.c `Screen* g_screen`, anim2.c/bigrender.c/
  bswater.c `MapHdr* g_map`, bigscreens.c `LevelMap* g_level_map`, bighelp.c
  `GameRec* g_game`, fpui3.c `ScrollMap* g_scroll_map`, coaster.c
  `Config* lpConfig`. This file needs only +0x10/+0x12 and calls it
  **`ViewCfg* g_view`**. Note legoland.h's `g_map` is a different type at the
  same address, so the name `g_map` is not available inside a file that
  includes legoland.h.
- **`g_res_path` is `char[]` here**, matching sysmisc.c. The all-drives prober
  writes into it; the ResPath prober only reads `[0]`.

## Callees and globals named for the first time

| address | name | note |
| --- | --- | --- |
| 0x00496570 | `PanFromOffset(int dx)` | `dx * 4` clamped to ±10000 |
| 0x00496540 | `VolumeFromDistSq(int d2)` | `g_sfx_master_db - d2/60`, clamped |
| 0x00492ca0 | `SetThemeInTransition(int theme)` | posts IMT opcode 3 when state is 1 or 2 |
| 0x004bf778 | `g_imt_state` | interactive-music thread state |
| 0x0079a6a0 | `g_imt_event` | the `SetEvent` handle the thread waits on |
| 0x0079a6a4 | `g_imt_cmd` | mailbox opcode |
| 0x0079a6a8 | `g_imt_cmd_arg` | mailbox argument |
| 0x0079a6ac | `g_imt_theme` | theme currently playing |
| 0x004bf7cc | `kThemeSame` | `"IMT:Theme was same %d, continuing\n"` |
| 0x004bf7a4 | `kAlreadyInTrans` | `"IMT:Already in transition, continuing\n"` |
| 0x004bf77c | `kChangeBeforeTrans` | `"IMT:Changing theme before transition\n"` |
| 0x004b85ec | `kCDFS` | `"CDFS"` — the real "is this a CD" test |
| 0x004b8630 | `"c:\\"` | the root-path template, shared by both probers |
| 0x004b85f4 | `kGettingInfo` | `"Getting Info on drive %s"` |
| 0x004b8610 | `kCheckingAllDrives` | `"Checking all drives (Mask = %d)"` |
| 0x004b85c8 | `kDriveHasCd` | `"Drive %s contains the correct CD"` |
| 0x0049f34b | `toupper` | CRT |
| 0x004ab210 | `__imp__GetDriveTypeA@4` | |
| 0x004ab214 | `__imp__GetVolumeInformationA@32` | |
| 0x004ab218 | `__imp__GetLogicalDrives@0` | |
| 0x004ab0f8 | `__imp__SetEvent@4` | 0x004ab0fc is `LoadLibraryExA` |

## Free follow-ups (outside this lane's table, should be cheap)

- **0x00492d80 (5 instructions)** is the IMT mailbox poster for opcode 1:
  `g_imt_cmd = 1; SetEvent(g_imt_event);`, no argument. It is the third caller
  of the mailbox and would close on the first compile.
- **0x00492ca0 `SetThemeInTransition` (15 instructions)** is `SetTheme`'s
  state-1/2 arm: `if (state == 1 || state == 2) { g_imt_cmd = 3;
  g_imt_cmd_arg = theme % 5; SetEvent(g_imt_event); }`. Same shape as the
  fall-through arm of `SetTheme`, which is already exact.
- **0x00496570 `PanFromOffset` (10 instructions)** and **0x00496540
  `VolumeFromDistSq` (16 instructions)** are both straight-line clamps, fully
  described above.
