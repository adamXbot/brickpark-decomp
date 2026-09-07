# Scope LL13 — dead (linker-retained) functions, 0x004511e0..0x00466070

Branch `scope/LL13` from `main` `7d756410`. New file `LEGOLAND/unref5.c`.
Object prefix `/tmp/sll13_`. Brief: `docs/SCOPE_LL13_unref_gamemain_bighelp.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x004514b0 | LockLogicalVolume | 55 | 100 | [OK] | FUNCTION |
| 0x004511e0 | VolumeDebugHookA | 1 | 100 | [OK] | FUNCTION |
| 0x004511f0 | VolumeLockNoOp | 1 | 100 | [OK] | FUNCTION |
| 0x00451200 | VolumeDebugHookB | 1 | 100 | [OK] | FUNCTION |
| 0x00451480 | OpenVWin32 | 9 | 100 | [OK] | FUNCTION |
| 0x004514a0 | CloseVWin32 | 4 | 100 | — (tooling) | WIP-FUNCTION |
| 0x00451410 | UnlockPhysicalVolume | 35 | 100 | [OK] | FUNCTION |
| 0x00451390 | LockPhysicalVolume | 39 | 64 | — | WIP-FUNCTION |
| 0x00451550 | UnlockLogicalVolume | 49 | 100 | — (tooling) | WIP-FUNCTION |
| 0x00451280 | UnlockAllPhysicalLocks | 88 | 48 | — | WIP-FUNCTION |
| 0x00451210 | ReleaseVolumeLocks | 35 | 100 | [OK] | FUNCTION |
| 0x004551a0 | MeasureWrappedTextHeight | 46 | 100 | [OK] | FUNCTION |
| 0x00455220 | PrintWrappedTextOnSurface | 120 | 100 | [OK] | FUNCTION |
| 0x00455de0 | FindCachedTextByString | 51 | 100 | [OK] | FUNCTION |
| 0x00466070 | PresentNoOp | 1 | 100 | [OK] | FUNCTION |
| 0x0045ade0 | DrawTileDebugOverlay | 291 | 71 | — | WIP-FUNCTION |
| 0x00453c20 | DDrawErrorPassThrough | 20 | 20 | — | WIP-FUNCTION |

**14 of 17 bodies are byte-exact** (408 of 846 instructions), of which
**11 print `[OK]`** under `audit.py` (355 instructions). Three exact bodies
cannot be *gated*: see "Tooling" below.
`audit.py` ends **PASS**, `relocs.py` reports **zero MISMATCH** over the 11
`// FUNCTION:` bodies (41 relocations, 0 unresolved), `/W3` is clean.

## Tooling — only ONE `__stdcall` body per file can pass `audit.py`

`tools/match.py`'s `obj_function_code` looks a name up as `X` or `_X` and,
failing that, falls back to the FIRST `.text` COMDAT of the object. A
`__stdcall` function's COFF symbol is `_X@N`, so it is never found by name;
`input2.c` already records this for `LegoLandWindowProc` and keeps it first
in its file. This scope has **three** `__stdcall` bodies (`ret 4`, `ret 8`,
`ret 0x10` are not reachable any other way under `/Gd`), so at most one of
them can be reached by that fallback.

`LockLogicalVolume` (0x004514b0, the largest at 55 instructions) is therefore
the first function defined in `unref5.c` and must stay there.
`CloseVWin32` (0x004514a0) and `UnlockLogicalVolume` (0x00451550) carry
WIP markers even though both are **0 mismatches over their full extent**
(4i/14B and 49i/132B), verified with `matchfull.py`/`audit.py` on a copy of
the file that has only that function in it (`/tmp/sll13_std.py` builds one
from the real file's header plus one body, so the C is byte-identical). If
`match.py` ever learns to strip the `@N` decoration, all three flip to
`[OK]` with no source change.

## Names chosen (nothing here is exported; every name is ours)

| address | name | why |
| --- | --- | --- |
| 0x004511e0 | `VolumeDebugHookA` | 1-byte `ret`, no caller, sysmisc2.c neighbourhood |
| 0x004511f0 | `VolumeLockNoOp` | 1-byte `ret`, called by 0x00451210 with the drive letter |
| 0x00451200 | `VolumeDebugHookB` | 1-byte `ret`, no caller |
| 0x00451210 | `ReleaseVolumeLocks` | unlocks everything on one drive then closes VWIN32 |
| 0x00451280 | `UnlockAllPhysicalLocks` | reads the lock count, then unlocks that many times |
| 0x00451390 | `LockPhysicalVolume` | 440Dh/0848h with parameter-block op 0 |
| 0x00451410 | `UnlockPhysicalVolume` | 440Dh/0849h, no parameter block |
| 0x00451480 | `OpenVWin32` | `CreateFileA("\\\\.\\vwin32", …)` |
| 0x004514a0 | `CloseVWin32` | `CloseHandle` wrapper, `__stdcall` |
| 0x004514b0 | `LockLogicalVolume` | 440Dh/xx4Ah, BL drive / BH level / DX permissions |
| 0x00451550 | `UnlockLogicalVolume` | 440Dh/xx6Ah, BL drive |
| 0x00453c20 | `DDrawErrorPassThrough` | switch over DDERR codes, every arm returns the input |
| 0x004551a0 | `MeasureWrappedTextHeight` | DT_CALCRECT measure of a wrapped column |
| 0x00455220 | `PrintWrappedTextOnSurface` | the same measure, then draws on the DD surface |
| 0x00455de0 | `FindCachedTextByString` | text-cache lookup keyed on the string alone |
| 0x00466070 | `PresentNoOp` | 1-byte `ret`, blitmisc.c neighbourhood |
| 0x0045ade0 | `DrawTileDebugOverlay` | the name mapbuild2.c already proposed |

## Globals and strings named for the first time

- `0x004b85c4` `g_vwin32` — the open VWIN32 handle, initialised to
  `0xffffffff` in `.data` (the four bytes immediately before
  `kDriveHasCd` at 0x004b85c8). `ReleaseVolumeLocks` resets it to -1.
- `0x004b8634` `kVwin32Device` — `"\\\\.\\vwin32"`, sitting between
  `"c:\\"` (0x004b8630) and `"Please insert the LEGOLAND CD-ROM…"`.

## Mechanics recovered

### The VWIN32 volume-lock library (0x004511e0 .. 0x00451550)

Eight functions are a Windows 95 raw-disk-access helper set, all going through
`DeviceIoControl(h, VWIN32_DIOC_DOS_IOCTL == 1, &regs, 28, &regs, 28, &cb, 0)`
with the 28-byte `DIOC_REGISTERS` block
`+0 EBX, +4 EDX, +8 ECX, +0xc EAX, +0x10 EDI, +0x14 ESI, +0x18 Flags`
(bit 0 of Flags is CF). Every call is INT 21h **AX=440Dh**, generic block
IOCTL, with the category/minor pair in CX:

| function | CX | parameters |
| --- | --- | --- |
| `LockPhysicalVolume` | 0x0848 | DS:DX -> `{BYTE op; BYTE nlocks}`, op 0 |
| `UnlockAllPhysicalLocks` | 0x0848 | op 2 reads `nlocks`, then op 1 that many times |
| `UnlockPhysicalVolume` | 0x0849 | BL = drive only |
| `LockLogicalVolume` | 0x484A then 0x084A | BL drive, BH level, DX permissions |
| `UnlockLogicalVolume` | 0x486A then 0x086A | BL drive |

The two logical-volume calls try **category 0x48** (the OSR2 FAT32 extension)
first and retry with **category 0x08** on any failure; the physical ones only
ever use 0x08. `UnlockAllPhysicalLocks` tolerates DOS errors 0xB0 and 1 on the
query call and reports success anyway. Drive numbers are 1-based (A: = 1),
which is what `ReleaseVolumeLocks`'s `toupper(letter) - 0x40` produces.
`ReleaseVolumeLocks(letter)` is the teardown: no-op hook, then (if the handle
is not `INVALID_HANDLE_VALUE`) unlock-all-physical, unlock-physical,
unlock-logical, `CloseHandle`, handle = -1.

Nothing in the shipped game calls any of them — res.c mounts the CD by volume
label (`RES_FindVolumeOnAnyDrive` / `RES_FindVolumeOnResPath`, sysmisc2.c).
This looks like an abandoned "lock the CD-ROM so Windows cannot eject it"
feature; it explains why `"\\\\.\\vwin32"` is in `.data` next to the CD
strings.

### The wrapped-text pair (0x004551a0, 0x00455220)

Both start with the identical measuring block: a throwaway
`CreateCompatibleDC(0)`, `SetBkMode(dc, 1)`, `SelectFont(dc, font)`,
`DrawTextA(dc, s, strlen(s), &rc, 0x450)` with `rc.right` seeded at
`width - 1`, then `DeleteDC`. **0x450 = DT_CALCRECT|DT_EXPANDTABS|DT_WORDBREAK**,
a combination no other matched body in the tree uses (bighelp/fpui2 use 0x410,
frontend2 0x425, movie 0x24). 0x004551a0 returns `rc.bottom - rc.top + 1`.

0x00455220 then offsets the measured rect by `(x, y)`,
`PushRenderingStatusAndUnlockVideoSurface`, takes a GDI DC on
`g_draw_surface` (vtable +0x44), selects in a clipping region built with
`CreateRectRgnIndirect(&g_clip_rect)`, redraws with **0x50** (the same flags
without DT_CALCRECT), then selects the old font and old region back, deletes
the region, releases the DC (vtable +0x68) and `PopRenderingStatus`. It is the
only body in the tree that clips a DrawText through an explicit region — the
live text painters clip by rectangle.

**ORIGINAL BUG (both, reproduced):** the font selected into the measuring DC
is never selected back out before `DeleteDC`; `SelectFont`'s result is
discarded. Harmless (the DC is scratch), and misc3.c's `MeasurePopUpTitle`
leaks the whole DC the same way.

### `FindCachedTextByString` (0x00455de0)

The weakest of the four walkers over the 0x20-byte `TextEntry` array at
0x006675c0: `FindCachedTextBox` (0x00455c80) matches w/h/format/ink/paper/font
and the text, `FindCachedText` (0x00455d40) everything but w/h, this one
**only the text**. Same shape as its siblings — cursor anchored at the +0x0c
text pointer, count re-read from the `volatile` global in the latch.

### `DrawTileDebugOverlay` (0x0045ade0)

The map-editor overlay that stamps the "blocked" tile sprite over every
visible cell whose RF byte (Cell +0x10) has bit 1 set, in
`g_tileset_id3`/`g_basic_tiles_data` colours at tint 0xff6868.

- Clip window = `{map->origin_x, map->origin_y, map->screen_w, map->screen_h}`
  (MapHdr +0x20/+0x22 and +0x00/+0x02) through `SetClipping` (0x0048a5c0).
- Tile size is read exactly as `GetTileCentre` reads it:
  `short h = g_tile_sprites[g_default_tile]->h; short w = (short)(h + h);`
  with `hh = (h+1)>>1`, `hw = (w+1)>>1`.
- Scroll is split with FOUR `idiv`s — `sx/w`, `sx%w`, `sy/h`, `sy%h` are four
  separate source expressions (VC6 does not CSE a div with its mod), where
  `sx = g_scroll_x>>8` and `sy = (g_scroll_y>>8) - hh`. The starting cell is
  `col = sy/h + sx/w - 3`, `row = sy/h - sx/w`.
- The sub-tile remainder `(rx, ry)` selects one of the diamond's four
  triangles: `q = (rx >= hw) + 1; if (ry > hh) q += 2;` and the switch on
  1..4 nudges the start by one diagonal step when the point falls outside the
  diamond:
  `1: rx < hw-2ry -> rx+=hw, col--, ry+=hh`;
  `2: rx >= hw+2ry -> rx-=hw, row--, ry+=hh`;
  `3: rx < hw+2(ry-h) -> row++, rx+=hw, ry-=hh`;
  `4: rx >= hw+2(h-ry) -> col++, rx-=hw, ry-=hh`.
  Cases 3 and 4 share the `ry -= hh` tail (VC6 merges it; both are spelled
  out in source).
- Then, per screen row `y` from `origin_y - 2h - ry` to `2h + screen_h` in
  steps of `h`, two diagonal scans of `col++/row--` across
  `origin_x - 2w - rx` to `2w + screen_w` in steps of `w`; the second scan
  starts one cell later and is drawn at `(x + hw, y + hh)`. After both, the
  outer cursor advances by `col+1, row+1`.
- **ORIGINAL BEHAVIOUR (reproduced):** an out-of-range cell only has its RF
  byte zeroed — the other 19 bytes of the local `Cell` still hold the
  PREVIOUS cell's contents. Harmless because nothing but `rf` is read.

### `DDrawErrorPassThrough` (0x00453c20)

Twenty instructions of switch skeleton over `MAKE_DDHRESULT` codes with every
arm and the default reaching the same bare `ret`. Decoded from the tables
(all of this is read, not guessed):

- byte index table at 0x00453c70, 0x51 entries, base 0x88760014 → decimal
  20..100; bucket-0 entries at indices 0, 0x14, 0x23, 0x4b, 0x50 → decimal
  **20, 40, 55, 95, 100** = `DDERR_CANNOTDETACHSURFACE`,
  `DDERR_CURRENTLYNOTAVAIL`, `DDERR_EXCEPTION`, `DDERR_INCOMPATIBLEPRIMARY`,
  `DDERR_INVALIDCAPS`. Bucket 1 is the default;
- both dword entries at 0x00453c68 hold 0x00453c66, the `ret`;
- singleton compares at 430 (`DDERR_SURFACEBUSY`), 222
  (`DDERR_NODIRECTDRAWSUPPORT`), 110 (`DDERR_INVALIDCLIPLIST`) and a
  `cmp eax,0x8876000a / jle` boundary at 10 (`DDERR_CANNOTATTACHSURFACE`);
- `lea ecx,[eax-0x88760078]` at 0x00453c60 is a **dead** index computation
  for a second cluster based at decimal 120 (`DDERR_INVALIDMODE`) whose
  dispatch VC6 folded away completely.

The membership of that 120.. cluster is not recoverable from the binary and it
is what decides how VC6 splits the search, so the committed body carries only
the nine codes actually read out (16 of 20 strict). See the levers below for
what does and does not keep an empty switch alive.

## Levers learned (with evidence)

- **A `__stdcall` body must be the FIRST function in its file to be gated.**
  `obj_function_code` tries `X` and `_X` only; `_X@N` falls through to "first
  `.text` COMDAT". Measured here on three bodies: each is 0 mismatches when
  it is first and a nonsense REJECT (compared against whatever body *is*
  first) otherwise. A file can gate at most one `__stdcall` function.
- **`memset` does not shrink around a later full-dword store.**
  `memset(&r, 0, sizeof r); r.reg_EBX = drive;` emits `mov ecx,7 / lea
  edi,[esp+4] / rep stosd`; the original is `mov ecx,6 / lea edi,[esp+8]`.
  The source really is `memset(&r.reg_EDX, 0, sizeof(r) - sizeof(r.reg_EBX))`
  — a partial memset over everything past the first field — and it must come
  BEFORE the `reg_EBX` store (after it, the store is scheduled up into the
  memset setup and 6 more instructions move). Exact at 0x00451410.
- **Loop-invariant values must NOT be named locals when the loop already
  needs four callee-saved registers.** `LockLogicalVolume` went from 44/55
  to **0/55** by deleting the `bx` and `dx` temporaries and writing
  `(unsigned short)(level << 8) | (drive & 0xff)` and `perms & 0xffff`
  directly at their store sites inside the loop. With the temporaries VC6
  caches `[__imp__DeviceIoControl]` in ebp and spills `bx`; without them it
  keeps `h`, `bx`, `dx`, `cat` in ebp/esi/edi/bl and re-loads the import from
  memory on every call — which is what the original does. Same fix did NOT
  apply to `UnlockLogicalVolume`, which only has one invariant and matches
  with the import cached in edi.
- **A retry loop's "give up" value must be assigned on the failure path, not
  by a `return 0` after the retry test.** `... if (ok) { rc = 1; break; }
  rc = 0; if (cat == 8) break; cat = 8;` emits `xor eax,eax` BEFORE
  `cmp bl,8` and exiles `mov eax,1` past the back edge — exactly the
  original. Written as `if (ok) return 1; if (cat == 8) return 0;` the
  `xor eax,eax` sinks into the epilogue and 12 of 49 instructions move
  (0x00451550), 44 of 55 at 0x004514b0.
- **A `char` parameter in the CALLEE's prototype lets the caller push the
  argument's home dword unwidened.** 0x00451210 does
  `sub al,0x40 / mov byte [esp+0x10],al / mov esi,[esp+0x10] / push esi`
  three times — a byte store read back as a dword. That is not a union or an
  aggregate: it is what VC6 emits when the callees are declared
  `f(void*, unsigned char)`, because the upper 24 bits are don't-care. The
  callees then read the argument as `mov eax,[esp+N] / and eax,0xff`
  (TY07 on an argument home). Exact first try at 0x00451210 and 0x00451410.
- **`short` locals used in int expressions spill their sign-extended value at
  the definition site; `int` copies of them do not.** `DrawTileDebugOverlay`
  dropped from 247/291 to **122/291** (and from 884 to the exact 885 bytes)
  by deleting `int th = h; int tw = w;` and using the `short h`/`short w`
  everywhere — the same `short h; short w = (short)(h + h);` idiom
  `GetTileCentre` uses. With the int copies VC6 keeps them in registers and
  spills late; with the shorts it emits `movsx ebp,ax / mov [esp+0x20],ebp`
  at the definition, which is RA12 and what the original does.
- **A free `volatile` read pins a parameter load below an intervening store.**
  0x00451390's `mov eax,[esp+0x2c]` sits AFTER the two `p` byte stores;
  written plainly, VC6 hoists it above them into ecx.
  `*(volatile unsigned long*)&drive & 0xff` at the definition site moved the
  first divergence from index 6 to index 8 (16 → 14 of 39). It does not fix
  the register, because eax still holds the memset's zero and VC6 reuses it
  for the `push 0` (`push eax`) instead of letting the load kill it.
- **VC6 deletes an empty `switch` outright at /O2 — unless every arm
  `return`s.** Measured on 22 DDERR cases: `break`-only arms, `goto out`
  arms, `return;` arms in a `void` function, and arms that assign to a
  dead local ALL compile to a bare `ret` (16 bytes of COMDAT). Arms that
  `return 0` in an `int` function keep the whole binary search, and arms that
  `return hr` keep it AND fold to a single bare `ret` (no `xor eax,eax`) —
  which is the only shape that can reproduce 0x00453c20. `return hr` with one
  case label per arm is constant-folded into `mov eax,<case value>` per arm;
  the arms must share ONE body for the fold to be impossible.
- **Declaration order really is irrelevant to spill-slot assignment — but
  aggregate-ness is not (FR01).** Three very different declaration orders
  (including fully reversed, and the two aggregates swapped) produced
  byte-identical code for `DrawTileDebugOverlay`. Wrapping the two cursor
  scalars in a struct (`struct { int c, r; } cur;`, address never taken, so
  VC6 still scalarizes them) moved five of the eleven spill slots into place
  and took the body from 122/291 to 90/291. Wrapping any of the other pairs
  (`rx`/`ry`, `hh`/`hw`, `savecol`/`saverow`) made it worse, so this is a
  per-pair lever, not a general one.
- **Statement order inside a switch arm decides where a reload is
  scheduled, not just the store order.** `cur.c--; rx += hw; ry += hh;`
  costs six fewer instructions than `rx += hw; cur.c--; ry += hh;` in
  0x0045ade0's case 1 (90 -> 84): with the decrement first, the `hh` reload
  stays at the end of the block where the original has it; with the store
  first, VC6 hoists it four slots. The emitted STORE order (rx then col) is
  the same either way.

## Residuals — what a stronger model could still move

- **0x0045ade0 `DrawTileDebugOverlay` (84/291 strict, rb 78, ob 65, first
  index 42).** Instruction count and byte count are exact and the body lines
  up shape-for-shape. Eight of the eleven 4-byte spill slots now agree; the
  residual is a THREE-WAY permutation of the remaining ones — the original
  has `w`@+0x24, `qx`(coalesced with the outer `row`)@+0x28 and
  `savecol`@+0x2c, ours has `savecol`@+0x24, `w`@+0x28, `qx`@+0x2c. That
  swaps which of `rx`/`col` is loaded into ecx at the top of the outer loop
  and costs one extra `mov` in the loop-A preheader, which is the whole
  `ob` residual. Measured and rejected: `savecol`/`saverow` as an aggregate
  (97), in (r, c) order (98), assigned in the other order (90),
  `rx`/`ry` as an aggregate (132), `qx`/`qy` as an aggregate (90, no change),
  `hh`/`hw` as an aggregate (107), every declaration order.
- **0x00451280 `UnlockAllPhysicalLocks` (46/88, first index 1).** Everything
  from the loop preheader to the latch (indices 46..67) already matches
  exactly. The single blocker is that the original SINKS `push ebx` past the
  two leading guards to just before `xor ebx,ebx` (so the early-failure
  epilogue pops only edi/esi/ebp). Tried and rejected: post-guard inner scope
  for `i` (no change), `break` to one trailing return (peels the first
  iteration, 71/88), `goto done` (ESCAPES), `do/while` inside
  `if (p.nlocks != 0)` (78/88), `while` form, `int rc` copy, top-level `int i`.
  BL13's demonstrated shapes did not reach it; something else is holding ebx
  live across the guards.
- **0x00451390 `LockPhysicalVolume` (14/39, first index 8).** Pure allocation
  rotation: the drive load must land in eax (killing the memset's zero, so
  the `lpOverlapped` argument becomes a literal `push 0`) instead of ecx.
  Eighteen spellings measured — store permutations, `p` as an array / a
  pointer / an initialiser / a second `memset`, `&drive` as the
  `lpBytesReturned` slot, `unsigned char` vs `unsigned long` parameter,
  volatile reads of every width, declaration order — all land on 14 or worse.
- **0x00453c20 `DDrawErrorPassThrough` (16/20).** Needs the membership of the
  DDERR cluster based at decimal 120; the emitted search tree is a function
  of the whole case list, so this is a search over subsets of the ddraw.h
  codes in (110, 222) — 120, 130, 140, 145, 150, 155, 160, 165, 170, 180,
  190, 200, 205, 210, 212, 215, 216, 220 are the candidates. Ten subsets
  tried; all give either one table (61 bytes) or three (83 bytes), never the
  original's one table plus three singleton compares plus the dead
  `lea ecx,[eax-0x88760078]` (71 bytes).
