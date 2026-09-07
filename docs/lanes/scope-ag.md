# Scope AG — certificate print path + WinMain shell (2026-09-07)

Branch `scope/AG`, baseline `main` `f2ff6920`. Two new files, nothing
else touched. Objects under `/tmp/sag_*`. `/W3` clean on both files.
`relocs.py` 0 `MISMATCH` on all three `// FUNCTION:` bodies (UNRESOLVED
lines are the two string literals and `__except_list`, the CRT absolute
that match.py resolves). **Three of three exact (2026-09-08).** WinMain was
held at WIP by the extent walker, not the C; `tools/match.py` now reads
the SEH scope table (below) and the body audits `[OK]` unchanged.

## Per function

| address | name | insns | % | audit | marker | residual |
| --- | --- | ---: | ---: | --- | --- | --- |
| 0x00451f40 | `KillControllers` | 9 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00451740 | `SaveScreenshotBmp` | 620 | 100 | `[OK]` | `// FUNCTION:` | — |
| 0x00453d10 | `WinMain` | 48 | 100 | `[OK]` 48i/143B | `// FUNCTION:` | walker fix (below) |

Names: gamemain.c already named 0x00451f40 `KillControllers` (frees
`g_controller`); render5.c already named 0x00451740 `SaveScreenshotBmp`.
The brief's `SaveCertificateBitmap` is render5.c's 0x00451e20 (exact),
which *calls* this body with `("EGC.bmp", g_cert_message, stamp)`.
0x00453d10 is `WinMain`.

## Mechanics recovered

**`KillControllers`.** If `g_controllers_ready` (0x00667104) is set,
`HeapFree_w(g_controller)` (0x00813b00) and clear the flag. Does not
null `g_controller` — SetupControllers' inverse only drops the ready
bit. Original behaviour, commented at the site.

**`WinMain(hinst, hprev, cmdline, ncmdshow)` stdcall** (`ret 0x10`).
`int r = -1` is the `or esi, -1 / mov [ebp-0x1c], esi` pair. Then
`ReadExeVersionString(g_exe_version)` and `GameMain(...)` under
`__try`. The filter is
`WriteExceptionReport(_exception_info(), "main thread")` — not the
handler body. WriteExceptionReport returns 0 (`EXCEPTION_CONTINUE_SEARCH`),
so the empty `__except` arm is never taken; the report is written and
the exception keeps searching. Both cdecl cleanups merge into one
`add esp, 0x14`.

matchfull is 48/48, 143B, byte-identical including the SEH prologue's
`fs:[0]` (scope U's `KNOWN_ABSOLUTE` on `__except_list`). audit used to
walk 31i/93B `ESCAPES`: the try body ends in `jmp` over the filter/handler,
which only the `.rdata` scope table at 0x004ab4e0 reaches
(`{-1, 0x00453d6d, 0x00453d7f}`). WriteExceptionReport /
ReportModuleDetails walked in full only because earlier forwards had set
`furthest` past their jmp. No C spelling moves the original's terminator,
so the fix went into the walker (integrator change, 2026-09-08):
`true_extent` recognises the VC6 SEH prologue (`push -1 / push
<scopetable> / push __except_handler3 / mov eax, fs:[0]`) and, at every
`mov dword ptr [ebp-4], K` that enters trylevel K, adds entry K's filter
and handler to `furthest`. Reading only the entered levels bounds the
table (the next SEH function's entries follow it contiguously, and
0x00453da0's table is immediately followed by 0x00454380's). All seven
SEH frames in the binary re-walk to the same extent except WinMain, which
becomes 48i/143B; audit `[OK]`, relocs 0 MISMATCH, `/W3` clean.

**`SaveScreenshotBmp(path, msg, stamp)`.** Not a screen grab and not a
BMP writer (fable-d's `_write` reading was `_read` 0x0049f4ca). It
enumerates the first local printer (`EnumPrintersA(PRINTER_ENUM_LOCAL,
NULL, 2, buf, 0x540, &needed, &returned)`), builds a 0x94-byte DEVMODE
(`dmSize=0x94`, `dmFields=DM_ORIENTATION`, `dmOrientation=LANDSCAPE`),
`CreateDCA(NULL, info->pPrinterName, NULL, &dm)`, `_open(path,
_O_BINARY, _S_IREAD)` (the 0x100 pmode is unused without `_O_CREAT`),
reads the 14-byte file header and 0x28-byte info header, `GlobalAlloc`
+ `GlobalLock` for the BITMAPINFO (palette when `biBitCount < 9`) and
the bits (`bfSize - bfOffBits`), `CreateDIBitmap`, then prints:
`GetDeviceCaps(RASTERCAPS) & RC_BITBLT`, `StartDocA` with
`"Lego certificate"`, `StartPage`, `CreateCompatibleDC` +
`SelectObject`, `StretchDIBits` SRCCOPY centred with signed
`pageW/8` and `pageH/64` margins, two `CreateFontIndirectA` faces
(`g_font_face` "Lego" at 0x004b86e0; 20pt FW_NORMAL then 8pt FW_LIGHT,
heights `-MulDiv(n, LOGPIXELSY, 72)`), `TextOutA` of `msg` (TA_CENTER)
and `stamp` (TA_LEFT), `EndPage` / `EndDoc`, frees. Truthy on success.

## Globals and symbols named for the first time

None new for KillControllers / WinMain (all named in input2.c /
gameframe.c / exceptlog.c / startup.c). First declared here for the
print path: `EnumPrintersA` thunk 0x0049e442 → [0x4ab344], `CreateDCA` [0x4ab0a8],
`CreateDIBitmap` [0x4ab0ac], `StartDocA` [0x4ab0a0], `StartPage`
[0x4ab090], `EndPage` [0x4ab084], `EndDoc` [0x4ab08c], `StretchDIBits`
[0x4ab088], `SetTextAlign` [0x4ab07c], `TextOutA` [0x4ab06c],
`LocalAlloc` [0x4ab13c], `LocalFree` [0x4ab244], `MulDiv` [0x4ab140],
`GlobalAlloc`/`Lock`/`Unlock`/`Free`. `_open` 0x0049f6c0, `_read`
0x0049f4ca, `_close` 0x0049f417. String `"Lego certificate"`
0x004b86e8. `g_font_face` 0x004b86e0 already named in screen.c.

## Original bugs reproduced

- `KillControllers` does not null `g_controller`.
- `WinMain`'s filter returns 0, so the empty handler never runs.
- `SaveScreenshotBmp` `DeleteObject(hbmp)` on the `CreateDIBitmap`
  failure path pushes 0 (hbmp is NULL).

## Extern-type divergences

- `GameMain(void*, void*, char*, int)` cdecl, as startup.c defines it.
- `WriteExceptionReport` / `EXCEPTION_POINTERS` as exceptlog.c.
- `_exception_info` declared locally; no `<excpt.h>`.
- `SaveScreenshotBmp` keeps render5.c's three-arg prototype.

## Levers, with evidence

- **`int r = -1`** is WinMain's `or esi, 0xffffffff / mov [ebp-0x1c],
  esi`. Uninitialised `r` dropped those three instructions (93.3%).
- **`__except (WriteExceptionReport(_exception_info(), "main thread"))`**
  puts the call in the *filter* (`ret` at 0x00453d7e), not the handler
  (`mov esp, [ebp-0x18]` at 0x00453d7f). `__except (1) { report(); }`
  swaps them.
- **A straight-line `__try` body's only path to its filter is the
  scope table.** WinMain's `true_extent` walk stopped at `jmp 0x453d89`
  (insn 30, 31i/93B) and flagged the compiled jmp to the epilogue as
  ESCAPES; no C spelling moves the original terminator (WriteExceptionReport
  and ReportModuleDetails only passed because forward branches in their
  try bodies reached past the jmp). Fixed in the walker: trylevel stores
  index the scope table (see "Mechanics"). A future SEH body with a
  straight-line try needs nothing special.
- **`returned <= 0` on an `unsigned long`** is `jbe`; `== 0` is `je`.
- **Field-by-field `BITMAPINFOHEADER` stores**, not `*dst = src`
  (`rep movsd`). The `cmp word [ebp+0xe], 9` hoists into the copy.
- **Palette shift count from the stack header** (`mov cl, [esp+0x46]`
  = `bih.biBitCount`), not from the dest (`[ebp+0xe]`).
- **DOCINFO: zero `lpszOutput` / `lpszDatatype` / `fwType` before
  `lpszDocName`**, so the name store sinks past the `StartDocA` pushes
  (`mov [esp+0x7c], "Lego certificate"`).
- **`xor ebp, ebp` as the zero register** once `info` / `bmi` named
  pointers are written as casts at the use site (esi had been the
  zero). hdc then lives in esi for the rest of the body.
- **Frame 0xb88 needs `char printers[0xA80]`** once
  `{returned, pBits, needed, memdc}` is a 16-byte addressed object
  (cbBuf is only 0x540). `0xA84` is the pre-struct tail; the leftover
  is unused high space, not a second enum buffer.
- **The 4-byte local shift was `pBits` sitting between `returned` and
  `needed`.** A 12-byte `{returned, pBits, needed}` (then memdc as the
  fourth member) packs that run at 0x28/0x2c/0x30/0x34 and slides
  BITMAPINFOHEADER / DOCINFO / DEVMODE / printers +4. A pad
  `extra[4]` coalesced into hdc/fd; `{returned, needed, pad}` jumped
  to the top of the frame (FR01).
- **TextOut Y is `pageH * 678 / pBmi->biHeight`**, not `bih.biHeight`.
  That is `idiv [pBmi+8]` and keeps pBmi live past `font1`, so those
  two no longer share a home (pBmi stays at 0x24, font1 at 0x14).
- **StretchDIBits destW must stay an unnamed arg.** After both
  `GetDeviceCaps` (IAT cached in ebp, then `mov ebp, eax` = pageH)
  the original `push SRCCOPY` / `cdq` / `and edx, 0x3f` / `push 0`
  starts signed `pageH/64` while eax still holds pageH, finishes
  `sar ecx, 6` after the bmi/bits/src pushes, then destH from ebp
  and destW as `lea eax,[edx+edx]; mov edx,[pageW]; sub edx,eax;
  push edx`. Precomputing the four named margins is 96.5%
  (pageW/8 first, both `sar` immediate). Inlining
  `pageW-2*(pageW/8)` finishes destW from xDest-in-eax before
  `mov edx, [pBmi]` (94.6%). A named `destW` used at TextOut
  backwards-propagates into the call: 97.7% (607/621) with the
  /64 split and delayed destW, but the sub dest becomes destW
  (`mov edx,eax; mov eax,[pageW]; sub eax,edx`) and hDib/pBmi
  homes swap. `destW = pageW` just before TextOut lifted that to
  99.8% (619/620, destW compute exact). TextOut X is `pageW / 2`
  — no destW local — so destW stays a compiler temporary and
  pageW remains the sub destination.
- **`pageW - (pageW/8) - (pageW/8)`, not `pageW - 2*(pageW/8)`.**
  The two-minus form is what interleaves `mov edx,[pBmi]` before
  `sar eax,3`. Algebraic identities (`+ -2*`, `-(2e-pageW)`,
  helpers) all CSE'd back to the 97.7% named-destW shape while
  destW was live at TextOut.
- **EnumPrintersA is a direct call to the import thunk, not
  `call [IAT]`.** Orig `call 0x49e442` (`jmp dword ptr [0x4ab344]`)
  is 5 bytes; `__declspec(dllimport)` emits `call [IAT]` (6 bytes)
  and was the last 1761-vs-1760 residual after destW closed.
  Drop dllimport; comment `/* 0x0049e442 */`. CreateDCA and the
  rest stay dllimport (`call dword ptr [0x4ab…]`).

## Verification

```sh
$PY tools/audit.py LEGOLAND/certificate.c   # KillControllers + SaveScreenshotBmp [OK]
$PY tools/audit.py LEGOLAND/winmain.c       # WinMain [OK] 48i/143B
$PY tools/relocs.py LEGOLAND/certificate.c  # 0 MISMATCH (1 UNRESOLVED string)
$PY tools/relocs.py LEGOLAND/winmain.c      # 0 MISMATCH (UNRESOLVED: __except_list x3, "main thread")
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sag_w3_cert.obj LEGOLAND/certificate.c
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sag_w3_winmain.obj LEGOLAND/winmain.c
```
