# Scope AG — certificate print path + WinMain shell (2026-09-07)

Branch `scope/AG`, baseline `main` `f2ff6920`. Two new files, nothing
else touched. Objects under `/tmp/sag_*`. `/W3` clean on both files.
`relocs.py` 0 `MISMATCH` on the one `// FUNCTION:` body.

## Per function

| address | name | insns | % | audit | marker | residual |
| --- | --- | ---: | ---: | --- | --- | --- |
| 0x00451f40 | `KillControllers` | 9 | 100 | `[OK]` | `// FUNCTION:` | first compile |
| 0x00453d10 | `WinMain` | 48 | 100 matchfull | 31i/93B ESCAPES | `// WIP-FUNCTION:` | SEH jmp-over-filter (below) |
| 0x00451740 | `SaveScreenshotBmp` | 620 | 96.5 | StretchDIBits schedule | `// WIP-FUNCTION:` | margin /64 split (below) |

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
`fs:[0]` (scope U's `KNOWN_ABSOLUTE` on `__except_list`). audit walks
31i/93B `ESCAPES`: the try body ends in `jmp` over the filter/handler,
which only the `.rdata` scope table reaches (scope N: 31i/93B walked →
48i/143B). WriteExceptionReport / ReportModuleDetails walk in full
because earlier forwards already set `furthest` past their jmp. No C
spelling moves the original's terminator. Marker stays WIP.

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
print path: `EnumPrintersA` [0x4ab344], `CreateDCA` [0x4ab0a8],
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
- **audit cannot `[OK]` a body whose only path to the filter is the
  scope table.** WinMain's try body is straight-line, so `true_extent`
  stops at the jmp-over-filter (31i/93B) and flags the compiled jmp to
  the epilogue as ESCAPES. matchfull, which uses the compiled COMDAT
  length, scores 48/48.
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
- **StretchDIBits margin schedule is the residual.** After both
  `GetDeviceCaps` (IAT cached in ebp, then `mov ebp, eax` = pageH)
  the original `push SRCCOPY` / `cdq` / `and edx, 0x3f` / `push 0`
  starts signed `pageH/64` while eax still holds pageH, finishes
  `sar ecx, 6` after the bmi/bits/src pushes, then destH from ebp
  and destW from reloaded xDest/pageW. Precomputing the four
  named margins is 96.5% (pageW/8 first, both `sar` immediate).
  Inlining all four expressions into the call emits that /64
  opening and the delayed `sar ecx, 6`, but destW
  (`pageW-2*(pageW/8)`) is finished from xDest-in-eax before
  `mov edx, [pBmi]` (94.6%, 581/614). `0*destH` and `(pBmi, destW)`
  were DCE'd to the same 94.6%; volatile pBmi 94.2%; volatile
  `pageH` at the call 95.0% (reload, not live eax) or 86.4% on
  the precomputed form. destW before the second GetDeviceCaps
  94.5% (breaks the IAT pair). No C spelling delayed destW
  without losing the split.

## Verification

```sh
$PY tools/audit.py LEGOLAND/certificate.c   # KillControllers [OK]; SaveScreenshotBmp [WIP] schedule
$PY tools/audit.py LEGOLAND/winmain.c       # WinMain [WIP] 31i/93B ESCAPES, mismatch 0
$PY tools/relocs.py LEGOLAND/certificate.c  # 0 MISMATCH (WIP body skipped)
$PY tools/relocs.py LEGOLAND/winmain.c      # 0 functions checked (WIP)
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sag_w3_cert.obj LEGOLAND/certificate.c
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sag_w3_winmain.obj LEGOLAND/winmain.c
```
