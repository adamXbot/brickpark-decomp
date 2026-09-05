# fable-b lane notes — `LEGOLAND/sysmisc.c`

Seven functions from five subsystems (display, resources, audio, input, 3D
people), ~600 instructions. **Five exact, two WIP at 91% and 89%**; `audit.py`
PASS, `/W3` clean. Written 2026-09-05 alongside the integrating session.

| address | function | insns | audit | residual |
| --- | --- | --- | --- | --- |
| 0x004966a0 | `UpdateSampleSource` | 66 | WIP | 6 — case-3 load scheduling |
| 0x00473970 | `CreateMouseDevice` | 66 | **[OK]** | — |
| 0x00463ef0 | `SetScreenDisplayMode` | 72 | **[OK]** | — (first compile) |
| 0x004401b0 | `UpdatePersonPos` | 74 | WIP | 8 — register allocation |
| 0x004515e0 | `RES_EnsureMounted` | 92 | **[OK]** | — |
| 0x004661d0 | `FlipPrimary` | 111 | **[OK]** | — (first compile) |
| 0x0047c6a0 | `LLIDB_UnLoadLLSData` | 119 | **[OK]** | — (first compile) |

Both WIPs are **byte-exact in length or one byte off**, so every encoding is
right and only ordering/allocation is left.

---

## Levers, with evidence

- **A shared tail that RELOADS two locals from the frame proves the arms all
  had to leave them in memory — and the way to make VC6 do that is a volatile
  STORE through a cast in every arm.** `UpdateSampleSource` ends in one block
  that does `mov edx,[esp+8] / mov eax,[esp+4] / push / push / push / call`.
  Written naturally (plain `p.x = …; p.y = …;` in each switch arm) VC6 keeps
  the values in registers, DUPLICATES that ten-instruction tail into every arm
  and emits 85 instructions for a 66-instruction function. Writing the arms'
  stores as `*(int volatile*)&p.x = e;` makes all four arms end identically,
  VC6 cross-jumps them into ONE tail, and the body is 66/66 with the arm
  layout, the case-1→case-3 cross-jump and the `ja` default target all exact.
  **Three cheaper things do NOT work**, each measured: passing the `Pos` BY
  VALUE to the tail callee (VC6 still forwards the stores, 85), reading the
  values back through a `static __inline` helper that takes a `Pos*` (VC6
  inlines the helper and forwards, 85), and a `goto`/label join with an
  explicit `default:` (85). Only forcing the stores works.

- **An 8-byte struct assignment and two field assignments are different
  objects even when the fields are adjacent.** `UpdateSampleSource` case 1
  reads a `Person3D`'s screen position at +0x1c/+0x20. `p.x = m->sx; p.y =
  m->sy;` makes VC6 RELOAD `b->person` after the first store (the store to an
  address-taken local may alias the pointer), which is one extra instruction
  and shifts the block's whole register rotation. Typing the pair as a `Pos`
  at +0x1c and writing `p = b->person->screen;` reads both fields before the
  first store and matches index for index. Hoisting the pointer into a local
  first (`Person3D* m = b->person;`) does not help — the reload is of
  `b->person`, not of `m`.

- **A free volatile READ is emitted FIRST in its basic block, not at its
  statement position.** This is the whole residual of `UpdateSampleSource`.
  Case 3 needs `s->src.pos.y` loaded on the near side of the volatile store to
  `p.x` (without the volatile read the load sinks below the store and costs
  12); the volatile read achieves that, but it also pins the load as the
  block's FIRST instruction, where the original has it third
  (`scroll_x / pos.x / pos.y`). Statement position is inert: the read emits
  first whether it is the block's first statement, sits after the `p.x`
  statement, or is spelled through an extra local (four spellings, all 6).
  Adding volatile reads on `g_scroll_x` or on `s->src.pos.x` to order the
  block instead **breaks the tail merge** (72 instructions), and so does the
  same on `g_scroll_x` alone — only a volatile read of `pos.y` is safe.

- **`test byte ptr [mem],1` on a byte that is field +3 of a word comes from
  `& 0x100` on the `unsigned short`, not `& 1` on the byte.** In
  `UpdatePersonPos`, `unsigned char flags63; … if (!(b->flags63 & 1))` gives
  `mov al,[esi+0x63] / … / test al,1` — an extra instruction, and the load
  also slides ahead of the function's pending `add esp,0x20`, shifting every
  later index. Declaring the field as `unsigned short flags62` at +0x62 and
  testing `(b->flags62 & 0x100) == 0` narrows to the original's single
  `test byte ptr [esi+0x63],1`. Worth 74 instructions instead of 75 and
  mismatch 22 → 10 in one edit. (Same family as DECOMP's "u16 flags `|= 0x100`
  narrows to a byte OR"; this is the read side of it.)

- **Reading two fields into locals BEFORE an intervening call is what puts
  them in callee-saved registers.** `UpdatePersonPos` loads `b->y` then `b->x`
  into ebx/ebp before `GetTileDimensions`. Spelled inline in the expressions
  that follow the call, VC6 loads them after it, drops from four callee-saved
  pushes to three and re-allocates the whole function (58 of 74 wrong). Two
  plain locals assigned before the call: 22, then 8 after the flag fix. The
  order matters too — `by` first, then `bx`.

- **VC6 homes an out-pointer local PAIR in the incoming parameter slots.**
  `UpdatePersonPos` is `sub esp,8` — eight bytes, which is only the
  address-taken `Pos`. Its `int tw, th`, address-taken by
  `GetTileDimensions(&tw,&th)`, live at `[R+4]` and `[R+8]`: the caller's own
  argument slots, free because both parameters were root-copied into esi/edi
  at entry and never re-read. Read `[esp+N]` by push depth or this region
  looks like a wild write into the caller's frame. (Confirms DECOMP's
  "uninitialised out-pointer locals are homed in dead argument slots" for
  written-then-read locals as well.)

- **A `sete al` + `test al,1` guard is `((x == 0) & 1) == 0`, and the `== 0`
  half is a LAYOUT lever.** `CreateMouseDevice` tests DIDEVCAPS::dwFlags with
  `xor eax,eax / test edx,edx / sete al / test al,1 / jne`. A plain
  `dwFlags != 0` gives a bare `test edx,edx / je` and is three instructions
  short. The `& 1` is what turns the byte test into a bit-0 test. The outer
  `== 0` (success arm inline, failure jumping to the ONE trailing `return 0`)
  is what keeps the failure block at the END: written as
  `if ((… ) & 1) goto fail;` VC6 inverts the branch and parks the failure
  block in the middle (12 of 66). `int t = (dwFlags == 0); if (!(t & 1))` is
  byte-identical to the `== 0` form.

- **Two near-identical arms of an if/else were written out IN FULL, error
  handler and tail included — the block layout is the proof.**
  `RES_EnsureMounted` is two CD-nag loops, one per branch of its argument.
  Writing the retry-box cancel handler once, or the trailing
  `if (minimised) { restore } return 1;` once after the if/else, puts the
  shared block BETWEEN the arms and loses each arm's own inline
  `test edi,edi / je` (90 of 92 instructions). Duplicating **both** into both
  arms is 92/92 on the next compile: VC6 cross-jumps the two cancel bodies
  into one copy parked after the first loop, and the two restore bodies into
  one copy parked after the second, while each arm keeps its own guard. Read
  the layout: a shared block sandwiched between two arms was written twice; a
  shared block after the second arm reached by a `jmp` from the first was too.

- **`sub esp,N` at the top with `mov dword ptr [esp+k], imm` stores and NO
  matching `add esp` is a local aggregate initialiser, not an argument list.**
  `FlipPrimary` opens with four such stores and `call LLSAuto` — but `LLSAuto`
  (0x0047d630, layervis.c) takes no arguments. They are
  `WinRect dst = { 0, 0, 640, 480 };`, and the same sixteen bytes are handed to
  `OffsetRect` and to `Blt` later. The missing cleanup is the tell.

- **`mov esi,[__imp__X] / call esi` needs no source construct — it is a plain
  import call inside a loop.** `timeGetTime` in `FlipPrimary` and
  `MessageBoxA` in `RES_EnsureMounted` both come out that way from ordinary
  `__declspec(dllimport) __stdcall` declarations; VC6 hoists the IAT load out
  of the loop by itself and the third, post-loop call reuses the register.

---

## Residuals (§6B triage)

- **`UpdateSampleSource` — 6 of 66, byte-exact (175/175 B).** First divergence
  at index 46. The three loads of case 3 come out `pos.y / scroll_x / pos.x`
  where the original has `scroll_x / pos.x / pos.y`; the following
  `sar/sub/store` differ only in register naming, which follows. Cases 0, 1
  and 2 and the whole shared tail are exact. **Scheduling**, forced by the free
  volatile read (see the lever above); ~20 spellings tried.
- **`UpdatePersonPos` — 8 of 74, 212 B vs 213 B.** First divergence at index
  20. (a) The original builds the isometric y-sum with `lea ecx,[ebx+ebp]`, a
  THIRD register; ours coalesces it into the x local's register as
  `add ebp,ebx` — the single byte of difference. (b) The x-scroll block uses
  ebx/eax where ours uses edx/ecx. `strict >> register-blind`, i.e.
  **allocation**. Everything tried and rejected: sum/diff temporaries in both
  orders, both statement orders, both `lea` operand orders, `tw * (…)` operand
  order, a `Pos` struct copy of the source pair, a dead self-assignment, and
  free volatile reads at six sites (`tw`, `th`, `pos.x`, `pos.y`,
  `g_screencfg->origin_x`, `b->dir`). The one that helps is the free volatile
  read on `pos.x` in the x-scroll statement: it closes the y-scroll block, 10
  → 8. Nothing reaches the `lea`.

---

## Mechanics recovered

- **`SoundSource::kind` (audio3.c's guess, now confirmed).** 0 = none →
  `ClearSampleSource`; 1 = bloke → its `Person3D`'s SCREEN position
  (+0x1c/+0x20), not its map position; 2 = map ref → `GetTileCentre` of
  `src.pos`; 3 = level xy → `src.pos` minus the 24.8 scroll (`>> 8`). All four
  end in **0x004965a0 `SetSampleScreenPos(Sample*, int x, int y)`** (first
  named here), which subtracts half the screen size read through 0x004bcbf4
  and pushes the pan/volume into the DirectSound buffer.
- **`CreateMouseDevice`.** `CreateDevice(GUID_SysMouse)` →
  `SetCooperativeLevel(hwnd, EXCLUSIVE|FOREGROUND = 5)` →
  `SetDataFormat(c_dfDIMouse)` → `GetProperty(DIPROP_GRANULARITY = MAKEDIPROP(3))`
  of `DIMOFS_Z` (dwObj 8, `DIPH_BYOFFSET`) into `g_wheel_granularity`
  (0x004bad54), which is therefore the wheel notch size (normally 120) →
  `GetCapabilities` with the DirectX 3 `DIDEVCAPS` size 0x2c, and a
  `dwFlags == 0` = "no device" test → `GetDeviceStatus(GUID_SysMouse) == DI_OK`
  as the return value.
- **`SetScreenDisplayMode`.** Full screen only (`g_windowed == 0`): tries the
  config's w×h at 16 bpp, then at 8 bpp, and returns 0 if both fail. Then
  `GetDisplayMode` into a 0x6c-byte `DDSURFACEDESC` and classifies
  `ddpfPixelFormat`: **`g_screen_depth` (0x00668088) is 0 for 8 bpp, 1 for
  16 bpp 555 and 2 for 16 bpp 565** — the 565 test is `dwGBitMask == 0x7e0`,
  spelled `(mask == 0x7e0) + 1`. Any other bit count returns 0.
- **`UpdatePersonPos`** (the whole isometric projection, in one place):
  `SetPersonDirection(p, b->dir)`, then
  `sx = ((b->x - b->y) * tilew) >> 9`, `sy = ((b->y + b->x) * tileh) >> 9`
  from `GetTileDimensions`, minus `Get_XScroll()` / `Get_YScroll()` (both
  `short`, sign-extended), then **`Person3D::depth` (+0x54) is that y BEFORE
  the origin adjust**, then `+ g_screencfg->origin_x` and
  `+ g_screencfg->origin_y - (b->height >> 1)` (the sprite is lifted by half
  its height), then `AdjustBlokePosition` (a fixed −0x4b/−0x4d fudge) and
  `SetPersonPosition`. Finally the animation frame is copied from the bloke's
  +0x74 **unless bit 0x100 of the +0x62 flags word is set**.
  Bloke fields recovered: +0x68/+0x6c = map x/y in 24.8, +0x70 = u16 sprite
  height, +0x72 = u8 heading, +0x74 = u8 animation frame, +0x62 flags bit
  0x100 = "keep the current frame".
- **`RES_EnsureMounted`.** Blocks until the CD is present, nagging with a
  RETRY/CANCEL box (`0x50015` = RETRYCANCEL|ICONHAND|SETFOREGROUND|TOPMOST,
  caption "CD Missing"), minimising the game the first time round and
  restoring it on the way out of either exit. Two probers:
  **0x00450f30 `RES_FindVolumeOnAnyDrive`** (walks the `GetLogicalDrives`
  mask, logs "Checking all drives (Mask = %d)") and **0x004510e0
  `RES_FindVolumeOnResPath`** (the drive named by `g_res_path` @0x00813b04);
  the message differs accordingly ("into the CD drive" vs "into drive %s").
- **`FlipPrimary`** (installed as `g_present` @0x004b9ca4 in full screen):
  `LLSAuto()`, then the game cursor sprite (`g_current_pointer` @0x00668148)
  stamped at `g_gfx_point` inside a
  `PushRenderingStatusAndLockVideoSurface` / `PopRenderingStatus` pair when
  `g_screencfg->cursor` (+0x1e) is set, then a **28 ms minimum frame period**
  spin on `timeGetTime`, then `GetClientRect` + `ClientToScreen` +
  `OffsetRect` to move a fixed 640×480 rect to the window's screen position
  and `Blt` `g_surface_78` (0x00668078) onto the primary with `DDBLT_WAIT`;
  on `DDERR_SURFACELOST` it `Restore`s the primary and blits once more, and
  returns 0 if the blit still fails. The tail is the frame accounting:
  a frame counter, a one-second FPS bucket and the per-frame tick delta.
- **`LLIDB_UnLoadLLSData`** — what an LLIDB element of type 0x10 / 0x1010
  actually parses into: **an ObjectClassList node** (the 0x00669240 chain
  castleobj.c calls `ObjectClassList`), unlinked through +0x00. Teardown
  order: per-class destructor `(*+0xac)(*+0xc4)`; `UnLoadObjectLibrary` when
  `+0x1c & 0x10000`; unlink; three sprites at +0x64/+0x68/+0x6c, each
  `LLSStop`ping the LLS at `sprite->image->lls` first **when the image's kind
  at +0x14 is 2 or 3** (the two animated kinds) and then `KillSprite`; a
  nested LLIDB element at +0x70 through `LLIDB_UnLoadData`; three owned heap
  blocks at +0x78/+0x7c/+0x80; the record itself; and finally `e->data = 0`.

---

## Original bugs reproduced (commented at the site)

- **`UpdateSampleSource` has no `default:`.** A `SoundSource::kind` outside
  0..3 falls straight through to `SetSampleScreenPos(s, p.x, p.y)` with both
  frame slots **uninitialised** — whatever the previous frame left there. The
  sourcing entry points only ever store 0..3, so it never fires.
- **`RES_EnsureMounted` never uses its `volume` argument's VALUE.** The
  parameter only selects which prober runs; both branches probe the
  hard-coded volume name `"LEGOLAND"` (0x004b86d0). res.c's only caller passes
  0, so the drive-specific branch is the one that runs in the shipped game.

---

## Extern-type and naming divergences (do NOT "align" these)

- **`UpdateSampleSource` returns `int`, not `void`.** audio2.c and audio3.c
  both declare it `void`; the body has three inline `xor eax,eax` guard
  returns and tails into `return SetSampleScreenPos(…)`. Defined `int` here.
  The two callers' `void` declarations are correct AS CALLER-SIDE LEVERS and
  must stay.
- **`UpdatePersonPos`** is declared in blokemisc.c as
  `void UpdatePersonPos(void* model, Bloke* b)`; defined here as
  `void UpdatePersonPos(BlokePerson* p, WalkBloke* b)`. Same two arguments,
  different pointer types.
- **`ClearSampleSource` (0x00496660)** is declared `int`-returning here
  (`case 0` tails into it and its result is this function's result);
  audio3.c declares it `void`.
- **0x004bcbf4 has four names in the tree already** — legoland.h `Map* g_map`
  (a DIFFERENT view: w/h at +0x14/+0x16), screen.c `ScreenCfg* g_screencfg`,
  gpu.c `Screen* g_screen`, anim2.c/goldrush.c `MapHdr* g_map`. This file uses
  ONE `ScreenCfg* g_screencfg` carrying every field it needs: w/h (+0x00/+0x02),
  `cursor` (+0x1e), `origin_x`/`origin_y` (+0x20/+0x22). Note legoland.h's
  `g_map` is a different type at the same address, so the name `g_map` is not
  available inside this file.
- **0x007cacd4 is named twice.** screen.c calls it `g_init_flag` because
  `InitScreen` zeroes it; `FlipPrimary` increments it on every presented
  frame, so this file names it **`g_frame_count`**. Recorded here rather than
  renaming screen.c.

## Callees and globals named for the first time

| address | name | note |
| --- | --- | --- |
| 0x004965a0 | `SetSampleScreenPos(Sample*, int, int)` | positional-audio pan/volume |
| 0x00450f30 | `RES_FindVolumeOnAnyDrive(const char*)` | walks the GetLogicalDrives mask |
| 0x004510e0 | `RES_FindVolumeOnResPath(const char*)` | probes the `g_res_path` drive |
| 0x0047fe70 | `WNDENV_Minimise()` | `ShowWindow(hwnd, SW_MINIMIZE)` |
| 0x0047fe80 | `WNDENV_Restore()` | `ShowWindow(hwnd, SW_RESTORE)` |
| 0x00668200 | `g_flip_time` | `timeGetTime` at the last blit |
| 0x006681f0 | `g_fps_base` | start of the current FPS second |
| 0x006681f4 | `g_last_frame` | `timeGetTime` at the last flip |
| 0x006681f8 | `g_fps_frames` | frames since `g_fps_base` |
| 0x007fea48 | `g_fps` | last completed second's frame count |
| 0x00669240 | `g_objclass_head` | = castleobj.c's `ObjectClassList` |
| 0x004ac0a0 | `GUID_SysMouse` | 0x004ac090 is `GUID_SysKeyboard` |
| 0x004ab578 | `c_dfDIMouse` | 0x004ab560 is `c_dfDIKeyboard`; 0x18 apart = `sizeof(DIDATAFORMAT)` |
| 0x004ab1f8 | `__imp__timeGetTime` | winmm |
| 0x004ab2a4 | `__imp__MessageBoxA@16` | |
| 0x004ab2ec | `__imp__GetClientRect@8` | |
| 0x004ab2dc | `__imp__ClientToScreen@8` | |
| 0x004ab29c | `__imp__OffsetRect@12` | |

## Free follow-up

**`CreateKeyboardDevice` (0x004738b0, 51 instructions)** is `CreateMouseDevice`
with the granularity block deleted and the keyboard GUID/data format — the
exact same `((caps.dwFlags == 0) & 1) == 0` guard and the same `goto fail`
shape. It is declared `extern` by input2.c and defined nowhere. It was left
alone only because it is outside this lane's table; it should close on the
first compile.
