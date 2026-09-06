# Scope U — the exception-report writer and the object-description loader: 9 of 9 exact (7 `[OK]` at hand-off, the two `__try` bodies promoted at merge)

**Status: complete; the tooling decision below was taken at merge
(2026-09-06): `tools/match.py` resolves the CRT's `__except_list` to the
absolute 0 (`KNOWN_ABSOLUTE`), both `__try` bodies audit `[OK]` and carry
`// FUNCTION:` markers.** The text below is the hand-off as written. Branch
`scope/U`, baseline `origin/main` `0e62e67c` (2026-09-06). Two new files,
`LEGOLAND/exceptlog.c` (8 functions, 766 instructions) and
`LEGOLAND/objdesc.c` (1 function, 316 instructions). Seven bodies are
`audit.py [OK]`; the two `__try` bodies agree with the original in every
instruction and every byte but the gate's normaliser scores their
`fs:[0]` accesses as mismatches (below), so they carry `// WIP-FUNCTION:`
with the exact residual. `/W3` clean on both files; `relocs.py` 0
`MISMATCH` (the unresolved lines are string literals). No existing file was
edited. Objects under `/tmp/su_*`.

## Per function

| address | name | insns / bytes | audit | marker | note |
| --- | --- | ---: | --- | --- | --- |
| 0x00454290 | `ReportWrite` | 22 / 75 | OK | `// FUNCTION:` | first try |
| 0x004542e0 | `ReportModuleLine` | 62 / 157 | OK | `// FUNCTION:` | 41 → 0: a `pagesize` local (below) |
| 0x00454380 | `ReportModuleDetails` | 110 / 380 | 3 residual | `// WIP-FUNCTION:` | 89 → 15 → 3; the 3 are `fs:[0]` (below) |
| 0x00454500 | `FormatFileTime` | 52 / 147 | OK | `// FUNCTION:` | first try |
| 0x004545a0 | `ReportSystemInfo` | 97 / 348 | OK | `// FUNCTION:` | first try |
| 0x00454700 | `ExceptionCodeName` | 64 / 487 | OK | `// FUNCTION:` | 1 → 0: `code == tab[i].code` operand order |
| 0x004548f0 | `PathFileName` | 14 / 27 | OK | `// FUNCTION:` | first try; the brief's `sub_4548f0`: `strrchr(path, '\\')` + 1 or path |
| 0x00453da0 | `WriteExceptionReport` | 345 / 1256 | 4 residual | `// WIP-FUNCTION:` | 65 → 58 → 4; the 4 are `fs:[0]` (below) |
| 0x0047c7f0 | `GetFreePlayItemInfo` | 316 / 929 | OK | `// FUNCTION:` | 280 → 137 → 0: two nesting levers (below); fpui2.c's name kept |

`WinMain` (0x00453d10) is not in this scope; it calls
`WriteExceptionReport(GetExceptionInformation(), "main thread")` from its
`__except` block.

## The gate and `__try` bodies (integrator decision)

These are the first `__try/__except` functions attempted in the tree. The
toolchain compiles them and VC6's SEH frame comes out exactly as the
original's: `push -1 / push <scope table> / push __except_handler3 / mov
eax, fs:[0] / push eax / mov fs:[0], esp`, the `[ebp-4]` try-level stores,
the inline filter (`mov eax,1 / ret`) and handler (`mov esp,[ebp-0x18]`)
blocks. Both bodies match the original instruction for instruction and
byte for byte (345/1256 and 110/380).

The residual is the three or four `fs:[0]` operands. The `/FAs` listing
spells them `fs:__except_list`: the object carries a relocation against the
CRT's absolute symbol `__except_list` (value 0) at each of those
displacements, `match.py` patches every relocated field to its sentinel, so
the compiled operand reads `fs:[0x990099]` and `norm()` turns it into
`fs:[<abs>]`, while the linked original has the resolved `fs:[0]`, which
`norm()` leaves alone. No C spelling can change a compiler-generated
prologue. Two one-line fixes in `tools/match.py` would admit them: resolve
absolute-symbol relocations to their value instead of the sentinel, or add
`fs:[0]` → `fs:[<abs>]` to `norm()`. Until one lands the markers stay WIP
with the residual stated; promote both once the gate prints `[OK]`.

## Mechanics recovered

**`WriteExceptionReport(EXCEPTION_POINTERS* ep, const char* where)`.**
Once per process (`g_report_written` 0x00667528). The exe path from
`GetModuleFileNameA(0)` gives the program name (file part, extension cut
at the last '.') and, with the file part overwritten by `exceptlog.txt`,
the report's path, opened with `CreateFileA(GENERIC_WRITE, 0, 0,
OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH)`; failure goes
to `OutputDebugStringA("Error creating exception report")`. Then, through
`ReportWrite` (wvsprintfA + WriteFile): `"%s caused %s in module %s at
%04x:%08x."` with the program name, `ExceptionCodeName`, the file part of
the module `VirtualQuery` finds at EIP ("Unknown" when none), CS:EIP;
`"Exception handler called in %s."`; `ReportSystemInfo`; for an access
violation with two parameters `"%s location %08x caused an access
violation."` with "Read from"/"Write to"; a blank line; `Registers:` in
four lines (EAX CS EIP EFLGS / EBX SS ESP EBP / ECX DS ESI FS / EDX ES EDI
GS); `Bytes at CS:EIP:` — `g_report_code_bytes` (16) bytes each under its
own `__try`, `"?? "` for one that faults; `Stack dump:` — dwords from
CONTEXT.Esp up to min(TIB.StackBase from `fs:[4]` by inline asm, Esp +
`g_report_stack_dwords` (0x800)), `g_report_per_line` (8) per line as
`"%08x: "` then `"%08x%s"` with " " or "\r\n", built in a 1000-byte line
flushed whenever the cursor passes `line + 1000 - 50`; the whole dump in
one `__try` whose handler writes "Exception encountered during stack
dump."; then `ReportModuleLine` and `CloseHandle`. Returns 0.

**`ReportSystemInfo`**: `GetSystemTimeAsFileTime` → `FormatFileTime` →
`"Error occurred at %s."`; `"%s (Version %s)"` with the exe path and
`g_exe_version` (0x0066752c, filled by gameframe.c's
`ReadExeVersionString`); `"Run by %s on machine %s."` (GetUserNameA /
GetComputerNameA, 200-byte buffers, "Unknown" on failure); `"%d
processor(s), type %d."` from SYSTEM_INFO; `"%d MBytes physical memory."`
= (dwTotalPhys + 0xfffff) >> 20.

**`ReportModuleLine`**: walks the whole 4 GB address space one
`VirtualQuery` at a time (`npages = (0x40000000 / pagesize) * 4`, stepping
by RegionSize / pagesize, or 0x10000 / pagesize when the query fails) and
reports each MEM_COMMIT region whose AllocationBase is above the last one
reported. **`ReportModuleDetails(f, base)`**: under `__try`,
`GetModuleFileNameA(base)`, the MZ and PE signature checks, then
`CreateFileA(GENERIC_READ)` for `GetFileSize` and `GetFileTime` (" - file
date is " + `FormatFileTime`), and `"%s, loaded at 0x%08x - %d bytes -
%08x%s"` with the path, base, size, the PE header's TimeDateStamp and the
date text (written even when the file could not be opened).

**`FormatFileTime(char* out, FILETIME ft)`** — the FILETIME by value,
converted in place by `FileTimeToLocalFileTime(&ft, &ft)`, then
`FileTimeToDosDateTime`; `"%d/%d/%d %02d:%02d:%02d"` = month, day, year
(1980 + (date >> 9)), hour, minute, seconds × 2; `""` on failure.
**`ExceptionCodeName`**: a 24-entry `{code, name}` table built on the
stack (the 48 immediate stores) searched linearly with an unsigned index;
"Unknown exception type" otherwise. The names run from "a Control-C"
(0x40010005) to "a Microsoft C++ Exception" (0xe06d7363); the table is in
the C.

**`GetFreePlayItemInfo(e, &text, &key, &parent)` (objdesc.c)** reads
`Objdesc\<e->image>` through RES: a size dword and the 0xd0-byte
description record (`ObjDesc`; only `key` at +0x26, `block1`/`block2` at
+0x50/+0x54 and `elem` at +0xc4 are touched — `e->data` is also zeroed),
then length-prefixed fields: two optional blocks (allocated, read, freed —
`block1` is zeroed after the `if`, `block2` in its `else`), a string, the
PARENT name (`ElemID`; kept only when the element's flag 0x10 is set), a
string, the THEME name (`LLIDB_FindElement(name, &g_fp_theme, 0)`; an
empty name fails the whole call), two strings (the first not
NUL-terminated), the ICON file name into `g_fp_icon_name` (0x007fdba0;
`LoadSprite(name, 4)`, falling back to `InstituteIcon.lls`), three strings,
and the description TEXT into a fresh `len + 1` heap block returned through
`*text`; `*key = d->key`. The record is `memset` before its NULL check (an
original bug reproduced).

## Globals and symbols named for the first time

`g_report_written` 0x00667528; `g_report_code_bytes` 0x004b8a88 (=16),
`g_report_stack_dwords` 0x004b8a8c (=0x800), `g_report_per_line` 0x004b8a90
(=8) — initialised .data ints, i.e. tunables in the original source;
`g_fp_icon_name` 0x007fdba0. `g_exe_version` 0x0066752c (gameframe.c
called it "the global at 0x0066752c"). Imports resolved with pefile:
GetModuleFileNameA [4ab1ec], CreateFileA [4ab258], OutputDebugStringA
[4ab1f0], VirtualQuery [4ab1f4], wvsprintfA [4ab2a8], lstrlenA [4ab1e8],
lstrcpyA [4ab240], WriteFile [4ab254], CloseHandle [4ab260], GetSystemInfo
[4ab1a8], GetFileSize [4ab25c], GetFileTime [4ab1dc],
FileTimeToLocalFileTime [4ab12c], FileTimeToDosDateTime [4ab1d0],
GetSystemTimeAsFileTime [4ab120], GetUserNameA [4ab000], GetComputerNameA
[4ab130], GlobalMemoryStatus [4ab134], wsprintfA [4ab298] (cdecl, as
schoolcar4.c). `strrchr` is CRT 0x004a0010, first declared here.

## Original bugs reproduced

- `GetFreePlayItemInfo`: `memset(d, 0, 0xd0)` runs before `if (d == 0)`.
- `GetFreePlayItemInfo`: the sixth string is read into `name` without a
  terminator.
- `ReportModuleLine`: the page count is `(0x40000000 / pagesize) * 4`, so
  a 4 GB walk; harmless but slow.

## Extern-type divergences (caller-side levers)

- `LLIDB_FindElement(const char*, FPElem**, unsigned int*)` with a local
  `FPElem` whose +0x08 is a byte (`mov dl, byte ptr [eax+8] / test dl,
  0x10`); legoland.h's LLElem has a dword there.
- `FormatFileTime(char*, FILETIME)` by value — the callers push both
  halves.
- All the Win32 structs are declared locally with only the used fields.

## Levers, with evidence

- **An EBP/SEH frame lays its locals out by VC6's symbol-hash bucket, and
  the NAMES are therefore load-bearing (RA/FR, new regime).** LEVERS FR10
  records this for `#pragma optimize("", off)`; it holds for `/O2` bodies
  with `__try` too: every local (including ones never stored) gets a home,
  homes are assigned bucket 0 first from ebp down, same bucket most
  recently declared first, block scopes after the enclosing scope. Probed
  with `/FAs` (`/tmp/su_dis/hashprobe.py`, anchors p,a..o = buckets
  0..15): single letters hash to `c mod 16`; two and three letters to
  `(4*c[n-2] + c[n-1] + 6) mod 16`; from four letters the first character
  re-enters (`aaaa` 15, `baaa` 0), so probe rather than compute. With the
  original's slot order read off its displacements, `WriteExceptionReport`
  needed 23 names in non-decreasing buckets with ties declared in reverse
  slot order: eol/fname/cut/ctx (0), culprit (1), ex (2),
  progname/path/report (3), textline/end (4), column (5),
  leeway/lpos/memq (9), pc (10), k/blamefile (11), violation (12),
  dwords/dir (13), stackend (14), nxt (15) — 58 → 4, frame 0xb40 → 0xb44.
  `ReportModuleDetails`: 15 → 3 by renaming `nt` → `pe` (bucket 2 → 12,
  below `h`) and adding the original's `dos` local (`mov [ebp-0x190], ebx`
  is a block-scoped `IMAGE_DOS_HEADER* dos = base`). Declaration order
  still sets the initialiser order (progname, culprit, column, textline,
  leeway) — both constraints were satisfiable at once.
- **A named `pagesize` local decides which candidate loses its register
  (ReportModuleLine).** With `si.dwPageSize` read at each use, VC6 gave ebp
  to the parameter `f` and reloaded the page size (41 mismatches); the
  original keeps the page size in edi for its three loop uses and re-reads
  `f` from `[esp+0x54]` at the one call. `pagesize = si.dwPageSize` after
  `GetSystemInfo`, declared with `last` before `page`, is exact.
- **Nesting the body under `if (f)` and the tail under `if (name[0])`
  (GetFreePlayItemInfo).** Leading `return 0` guards gave three inline
  epilogues (280). `if (f) { … if (d == 0) { close; return 0; } … }
  return 0;` merges the first failure into the trailing return (137); then
  `if (name[0]) { …; return sprite; } RES_CloseFile(f); HeapFree_w(d); }
  return 0;` lets the late failure fall through into that same trailing
  return, which is the original's exiled block at 0x47cb75 — 0. The `d ==
  0` guard keeps its own inline copy (`xor eax,eax` before the pops).
- **`code == tab[i].code`** gives `cmp edx, [ecx]`; the reverse gives `cmp
  [ecx], edx` (ExceptionCodeName's only miss).
- **`rw = "Read from"; if (info[0]) rw = "Write to";`** is `mov eax, A /
  test / je / mov eax, B`; the ternary tests first and loads after.
- **`char date[100] = ""` inside the `if` block** (ReportModuleDetails):
  the init runs after the guard, where the original has it; the same idiom
  at function scope in WriteExceptionReport runs in the prologue, also as
  the original.
- **`__asm mov eax, dword ptr fs:[4]` / `__asm mov stackend, eax`** is the
  only way to the original's `mov eax, fs:[4] / mov [ebp-0xb50], eax`;
  `NtCurrentTeb()` would read fs:[0x18].
- **Frame ascending by declaration for esp-frame aggregates (FR04) held
  for GetFreePlayItemInfo** — `len, sprite, name, desc1, path, desc2`
  declared in that order land at F+0, +4, +8, +0x108, +0x208, +0x308 on
  the first compile — and for ReportSystemInfo's nine locals.
- **Inert / not needed**: no volatile anywhere; `wvsprintfA(buf, fmt,
  (char*)(&fmt + 1))` is the va_start; the lstrcpyA IAT cache in edi
  appeared by itself (CC10).

## Verification

```sh
$PY tools/audit.py LEGOLAND/exceptlog.c   # 6 x [OK], 2 x [WIP ] (fs:[0] residual 3 and 4), PASS
$PY tools/audit.py LEGOLAND/objdesc.c     # 1 x [OK], PASS
$PY tools/relocs.py LEGOLAND/exceptlog.c  # 0 MISMATCH; the two WIP bodies are SKIPPED by the gate
$PY tools/relocs.py LEGOLAND/objdesc.c    # 0 MISMATCH
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/su_w3.obj LEGOLAND/exceptlog.c
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/su_w3.obj LEGOLAND/objdesc.c
```
