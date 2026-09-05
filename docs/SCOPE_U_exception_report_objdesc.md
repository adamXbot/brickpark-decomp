# Scope U — the exception-report writer and the object-description loader (2026-09-06)

> **Status: OPEN, unclaimed.** Branch `scope/U`. Notes: `docs/lanes/scope-u.md`.
> Object prefix `/tmp/su_`. Any agent. Cut from inventory groups 14 and 24.

**Read `docs/PARALLEL_CONTRACT.md` first; it carries everything not written
here** — including the relocation step of the gate
(`$PY tools/relocs.py LEGOLAND/<file>.c`, zero `MISMATCH` lines).

NEW-FUNCTION scope, **9 functions, ≈1082 instructions**, two new
files. Nothing here is reached by the game loop: the first file is WinMain's
`__except` handler's report writer, the second is called once by
`InitFreePlayLists` (fpui2.c).

## `LEGOLAND/exceptlog.c` — `exceptlog.txt` (≈766 insns)

WinMain's `__except` block calls `WriteExceptionReport` (0x00453da0, 345i)
with the exception record: it opens `exceptlog.txt` ("Error creating
exception report"), writes the exception code's name (`ExceptionCodeName`:
"a Control-C", "a Control-Break", "a Datatype Misalignment", "a Breakpoint",
"an Access Violation" — "Read from"/"Write to" — "an In Page Error", …),
the faulting module and its file date (" - file date is ",
`FormatFileTime` "%d/%d/%d %02d:%02d:%02d"), system information
(`ReportSystemInfo`, "Unknown"), and the register dump, through sixteen
`ReportWrite` calls. Imports are the file/module/version APIs at the IAT
slots the table lists (resolve them with `pefile` as scope Q did; the tree's
`__declspec(dllimport) … /* [0x4ab...] */` spelling). This body has no SEH
of its own and walks to its full extent (`docs/lanes/scope-n.md`).

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x00453da0 | `WriteExceptionReport` | 345 | called by 0x00453d10 [unmatched]. calls `0x004548f0` x2, `0x004a0010`, `0x00454700`, `0x00454290` x16, `0x004545a0`, `0x004542e0`; strings "Unknown", "own", "exceptlog.txt", "Error creating exception report"; globals `0x00667528`, `g_alloc_tag`; IAT [4ab1ec] [4ab258] [4ab1f0] [4ab1f4] |
| 0x00454290 | `ReportWrite` | 22 | called by 0x00453da0 [unmatched] (+3 more). IAT [4ab2a8] [4ab1e8] [4ab254] |
| 0x004542e0 | `ReportModuleLine` | 62 | called by 0x00453da0 [unmatched]. calls `0x00454290`, `0x00454380`; IAT [4ab1a8] [4ab1f4] |
| 0x00454380 | `ReportModuleDetails` | 110 | called by 0x004542e0 [unmatched]. calls `0x00454500`, `0x00454290`; strings " - file date is "; globals `g_alloc_tag`; IAT [4ab1ec] [4ab258] [4ab25c] [4ab1dc] |
| 0x00454500 | `FormatFileTime` | 52 | called by 0x00454380 [unmatched] (+1 more). strings "%d/%d/%d %02d:%02d:%02d"; IAT [4ab12c] [4ab1d0] [4ab298] |
| 0x004545a0 | `ReportSystemInfo` | 97 | called by 0x00453da0 [unmatched]. calls `0x00454500`, `0x00454290` x5; strings "Unknown"; globals `0x0066752c`; IAT [4ab120] [4ab1ec] [4ab000] [4ab130] |
| 0x00454700 | `ExceptionCodeName` | 64 | called by 0x00453da0 [unmatched]. strings "a Control-C", "a Control-Break", "a Datatype Misalignment", "a Breakpoint" |
| 0x004548f0 | `sub_4548f0` | 14 | called by 0x00453da0 [unmatched]. calls `0x004a0010` |

## `LEGOLAND/objdesc.c` — `Objdesc\%s` (≈316 insns)

| address | provisional name | insns | reached by; evidence |
| --- | --- | ---: | --- |
| 0x0047c7f0 | `LoadObjectDescriptions` | 316 | called by InitFreePlayLists (fpui2.c). calls `sprintf`, `RES_OpenFile`, `HeapAlloc_w` x4, `RES_CloseFile` x3, `RES_ReadFile` x28, `HeapFree_w` x4, `ElemID`, `LLIDB_FindElement`, `LoadSprite` x2; strings "Objdesc\%s", "InstituteIcon.lls"; globals `0x007fdba0`, `g_fp_theme` |

`InitFreePlayLists` (fpui2.c, matched) is the only caller and documents the
arguments; the body `sprintf`s `Objdesc\%s`, opens it through RES, reads it
in 28 `RES_ReadFile` calls into four heap blocks, looks up `ElemID`/
`LLIDB_FindElement` and loads `InstituteIcon.lls`.

## Owned elsewhere — do not create or edit

Every other new file of scopes R–X; Codex-F's files; every file in scopes
F, G, H; every existing `.c`.
