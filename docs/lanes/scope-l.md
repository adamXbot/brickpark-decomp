# Scope L — relocation sweep complete

**Status: 100% of the assigned sweep delivered.** Branch `scope/L`, source
baseline `origin/main` **`3601892`** (2026-09-05). The only deliverables are
`tools/relocs.py` and this report. All 2,355 exact-marker functions were
inspected; **197 source files compiled, zero failures, zero skipped functions**.
No C bodies, existing tools, shared progress files or scope briefs were edited.

**77 strict address differences in 20 functions.** Classification identifies
**22 positions in 7 functions requiring corrective integration review**, and
**55 positions in 13 functions attributable to computation/copy/block ordering**
(the floating-point case has a caveat below). There are **1,255 unresolved
positions in 372 functions**. Unresolved does not mean matched or mismatched.
No resolved direct-call target differs from its annotation/reference.

## Method and limits

The tool compiles each source once using the same `LEGOLAND_CL` wrapper,
`ALPHATEAM_VC6_ROOT` and `/nologo /c /O2 /Gy /Gd` as `audit.py`. Each invocation
owns a temporary `/tmp/sl_<pid>_*` directory; objects are deleted on exit.
It selects the actual decorated COFF function symbol and reuses
`match.py`'s `load_exe`, `rva2off`, `obj_function_code`, `true_extent`,
`compiled_body` and `compare`. The normalized full-extent gate must pass
before any instruction-index comparison is trusted.

The independent COFF reader preserves symbol indices across auxiliary
records, reads the function's section/value and its raw relocation addends,
and locates each relocation's actual 32-bit immediate/displacement operand.
Two relocated operands in one instruction are checked independently.
`DIR32` resolves to symbol VA + signed addend; `REL32` compares that target
with the original displacement plus the address immediately after the field;
`DIR32NB` restores the original image base. All arithmetic wraps to 32 bits.
These relocation kinds follow Microsoft's
[PE/COFF specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#intel-386-processors).

Symbol evidence comes from source-local address comments and function markers,
quoted local headers, exported symbol names, and the explicit global aliases
in DECOMP's export table. Six-digit addresses, multiline declarations,
next-line trailing comments, paired declarators and function pointers are
supported. Local annotations take precedence over cross-file references;
conflicts remain unresolved. Cross-file lookup is restricted to external COFF
symbols so a static helper cannot borrow an unrelated export's address.
IAT slots stay distinct from direct callees; only explicitly identified IAT
comments resolve `__imp__` symbols. In-function labels may be mapped through
the already verified byte layout; this is never extrapolated into tables.

Limits:

- This is an audit of **emitted code relocations**, not a linker or proof of
  gameplay correctness. Hardcoded addresses, fully assembled self-calls/local
  branches, relocation-free immediates, data-section initializers and jump-table
  contents are outside its coverage. The original extent walk is shared with
  the existing gate, so its limitations are inherited.
- Address comments/reference tables are evidence, not ground truth. A hit can
  expose a stale annotation instead of wrong C logic. The schoolcar sentinel
  below is a concrete example. The parser is intentionally conservative;
  unsupported C/preprocessor forms and unnamed/ambiguous symbols remain
  unresolved rather than inferred from the operand being tested. It does not
  evaluate general preprocessor conditions or parse system headers.
  A misleading callee name whose local address comment agrees with the
  original operand can therefore escape this check; cross-file name/comment
  consistency is a separate review.
- Strings, floating-point literals and jump tables without an independent
  address binding remain unresolved even when their contents look familiar.
  No matching address is inferred from the original target itself.
- Strict positional differences can preserve ordinary sequential behavior:
  commutative operands and independent stores/copies are reported as hits but
  classified separately. These assessments do not certify concurrent access,
  memory-mapped I/O, fault timing or unusual floating-point values.
- `--all` visits exact `// FUNCTION:` markers in `LEGOLAND/*.c`; WIP bodies
  are excluded. A compile failure or an unaligned/unknown extent is reported
  as skipped and the sweep continues. The present sweep skipped nothing.

## Reproduce

```sh
export LEGOLAND_CL='/Users/systemadmin/Downloads/alpha team/alphateam/tools/wibo-msvc/cl'
python3 tools/relocs.py --self-test
python3 tools/relocs.py LEGOLAND/pathmisc2.c NewMechanicOrder 0x004995d0
python3 tools/relocs.py --all --json /tmp/sl_sweep.json
```

The brief's `/Documents/...` wrapper and `~/.venvs/legoland/bin/python` are
absent on this machine. The already installed Downloads wrapper, existing
`toolchain` symlink and `/opt/homebrew/bin/python3` (Capstone 5.0.7) were used;
no compiler or environment was installed. The original executable SHA-256 is
`c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`.
The tool honors the same environment override elsewhere. No original binary,
object or game asset is committed.

Each stdout hit/unresolved line includes file, function VA, **zero-based**
instruction index, original target, resolved target, COFF symbol and addend.
Progress goes to stderr. Optional JSON also records every checked function,
per-function counts, all hit/unresolved rows, and compile/extent failures.
Exit status is a bitmask: `1` for mismatches, `2` for unresolved/skipped work,
`3` for both, `0` only when every checked relocation resolves and agrees.
The completed full sweep correctly exits **3**; this is a diagnostic result,
not a tool crash.

## Complete sweep counts

| Measure | Count |
| --- | ---: |
| Files compiled / failed | 197 / 0 |
| Exact functions attempted / checked / skipped | 2,355 / 2,355 / 0 |
| Emitted code relocation positions | 21,097 |
| Resolved, same target | 19,765 |
| Resolved, different target | 77 |
| Unresolved positions | 1,255 |
| Functions with at least one strict hit | 20 |
| Functions with unresolved positions | 372 |
| Functions with no emitted code relocation | 258 |
| Functions with neither hits nor unresolved positions | 1,970 |

The last category includes the 258 relocation-free functions and is not a
claim that all of their behavior was independently verified. Hit and
unresolved function counts overlap.

## Classifications and worked examples

| Class | Classification | Functions | Positions | Interpretation |
| --- | --- | ---: | ---: | --- |
| A | Swapped callback arguments | 1 | 2 | Corrective review |
| B | Packed-record field displacement | 2 | 3 | Corrective review |
| C | Wrong same-size global references | 2 | 11 | Corrective review |
| D | Equivalent switch-block order | 1 | 3 | Equivalent case-to-value mapping |
| E | Incorrect base address in declaration | 2 | 6 | Corrective review |
| F | Independent copies or equal-value stores reordered | 5 | 20 | Equivalent sequential results |
| G | Arithmetic operands exchanged | 5 | 18 | Equivalent sequential results |
| H | Equality-only comparison operands exchanged | 1 | 2 | Equivalent sequential results |
| I | Aggregate copy and floating-point operand order | 1 | 12 | Same ordinary-value result; FP caveat |

Worked examples (the complete per-position ledger follows):

- **A — callback arguments:** `InitPopUpInfo` i351 originally pushes
  `0x00473310` (PU_ToolA), while ours pushes `0x004731e0` (PU_ToolB).
  The next argument is reversed too: distinct popup actions receive the
  wrong callbacks.
- **B — layout:** `PrintSavedGameDetails` i97 originally reads byte
  `[0x0080ffe5]`; ours reads `g_cur_profile + 0x48 = 0x0080ffe8`.
  The intervening union is four bytes wide even under packing.
- **C — global identity:** `PlaneRide_Create` i34 stores EDX at
  `0x0062fe84`; ours stores it at `_g_plane_bnv0 = 0x0062fe90`, aliasing
  the source pointer rather than the distinct destination.
- **D — switch-block order:** `GetGFXFName` i55 loads `0x004b81cc`
  (index 3); ours loads `0x004b81d4` (index 5). The original jump table at
  `0x0044dfdc` associates these physical blocks with different case values:
  cases 4/5/6 still select directory indices 5/3/4, exactly like the source.
  Physical block order alone cannot establish a wrong case mapping.
- **E — declaration metadata:** `AnyCoasterRegionFullyInside` i0 loads
  `[0x00829a54]`; the declared base plus 0x18 resolves to `0x008299b8`.
  The declaration's own prose contradicts its leading base address.
- **F — copy ordering:** `ReadGameButtons` i294 reads map_y at
  `0x00813a68`; ours reads map_x at `0x00813a64`. Its matching store also
  moves, so click_x still receives map_x and click_y still receives map_y.
- **G — arithmetic:** `DoMapAI` i41 loads `0x00832824` then i42 multiplies
  by `[0x0083281c]`; ours exchanges these two addresses. The integer product
  is the same.
- **H — equality:** `UpdateControllerFromKeyboardData` i205 originally
  loads the `:IMPROVISE` string at `0x004bae30`; ours loads the ring-buffer
  tail at `0x00668d9e`. The complementary literal operand is unresolved;
  comparison only tests equality, which is symmetric.
- **I — aggregate order:** `SetupTrackDrawView` i2 loads source.b at
  `0x004b5cb0`; ours loads source.a at `0x004b5cac`. Destination stores and
  subsequent loads move with them, retaining the final four-field copy.
  Its swapped floating-point inputs are not certified for NaN behavior.

The synthetic wrong-callee test also changes a resolved REL32 target from
`0x00401104` to `0x00401204` and is detected. **There are zero differing resolved
direct-call targets under this tool's metadata-based resolution.** The initial apparent callees
in catapult/render2/renderlist were parser errors caused by next-line comments;
the parser was corrected, regression-tested and the entire sweep rerun.
The nearby-function-comment false hit on layervis.c's `kBadSprite` was likewise
removed; that unannotated string now remains unresolved.

## Every function with hits, and every mismatching position

All functions below retain their baseline `// FUNCTION:` markers and pass
the normalized full-body gate. Their strict relocation identities differ.
Each heading supplies the source file and original function address; each
row supplies the instruction index, original operand (including target),
COFF symbol/addend and resolved target. All rows inherit the classification
stated immediately above their table.

### `ReadGameButtons` — `LEGOLAND/bighelp.c` — `0x00452460`

Class **F**: Independent copies or equal-value stores reordered. 332 instructions / 1313 bytes; normalized gate passed.

The x/y loads and corresponding destination stores both exchange registers. Each click coordinate still receives its matching map coordinate; no source write or call intervenes.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 294 | `mov eax, dword ptr [0x813a68]` | `_g_input +0x24` | `0x00813a64` |
| 295 | `mov edx, dword ptr [0x813a64]` | `_g_input +0x28` | `0x00813a68` |
| 296 | `mov dword ptr [0x813a80], eax` | `_g_input +0x3c` | `0x00813a7c` |
| 299 | `mov dword ptr [0x813a7c], edx` | `_g_input +0x40` | `0x00813a80` |

### `InitPopUpInfo` — `LEGOLAND/bighelp.c` — `0x00470bb0`

Class **A**: Swapped callback arguments. 374 instructions / 1466 bytes; normalized gate passed.

The original pushes PU_ToolA (0x00473310), then PU_ToolB (0x004731e0), so cdecl calls InitPopUpTools(PU_ToolB, PU_ToolA). Source calls InitPopUpTools(PU_ToolA, PU_ToolB), exchanging the two installed callbacks. Definitions in fpui3.c/uimisc.c and the ok_fn/close_fn parameters in popup2.c confirm the identities.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 351 | `push 0x473310` | `_PU_ToolB +0x0` | `0x004731e0` |
| 352 | `push 0x4731e0` | `_PU_ToolA +0x0` | `0x00473310` |

### `PrintSavedGameDetails` — `LEGOLAND/bigscreens.c` — `0x0048dd00`

Class **B**: Packed-record field displacement. 313 instructions / 952 bytes; normalized gate passed.

CurProfile.save_slot is a union containing a DWORD. Packing removes alignment padding but does not shrink the union: the following f45 byte lands at +0x48, not its documented/original +0x45. Both save-type reads use 0x0080ffe8 instead of 0x0080ffe5.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 97 | `mov al, byte ptr [0x80ffe5]` | `_g_cur_profile +0x48` | `0x0080ffe8` |
| 113 | `cmp byte ptr [0x80ffe5], 2` | `_g_cur_profile +0x48` | `0x0080ffe8` |

### `InitGameInterface` — `LEGOLAND/bigscreens.c` — `0x004749d0`

Class **B**: Packed-record field displacement. 316 instructions / 1276 bytes; normalized gate passed.

The same bigscreens.c CurProfile union-size error moves this save-type read from +0x45 to +0x48. Shared root cause with PrintSavedGameDetails; changing the base address would be the wrong fix.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 274 | `mov al, byte ptr [0x80ffe5]` | `_g_cur_profile +0x48` | `0x0080ffe8` |

### `DoMapAI` — `LEGOLAND/bigsim.c` — `0x00462ef0`

Class **G**: Arithmetic operands exchanged. 272 instructions / 958 bytes; normalized gate passed.

Four integer multiplies exchange their two memory inputs. Each pair produces the same low 32-bit product, with no intervening write to either input.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 41 | `mov ecx, dword ptr [0x832824]` | `_g_map_ai +0x1c` | `0x0083281c` |
| 42 | `imul ecx, dword ptr [0x83281c]` | `_g_map_ai +0x24` | `0x00832824` |
| 61 | `mov ecx, dword ptr [0x832850]` | `_g_map_ai +0x44` | `0x00832844` |
| 63 | `imul ecx, dword ptr [0x832844]` | `_g_map_ai +0x50` | `0x00832850` |
| 72 | `mov ecx, dword ptr [0x8328d4]` | `_g_map_ai +0xc8` | `0x008328c8` |
| 73 | `imul ecx, dword ptr [0x8328c8]` | `_g_map_ai +0xd4` | `0x008328d4` |
| 82 | `mov ecx, dword ptr [0x832900]` | `_g_map_ai +0xf4` | `0x008328f4` |
| 83 | `imul ecx, dword ptr [0x8328f4]` | `_g_map_ai +0x100` | `0x00832900` |

### `UpdateControllerFromKeyboardData` — `LEGOLAND/input.c` — `0x00473c10`

Class **H**: Equality-only comparison operands exchanged. 283 instructions / 1112 bytes; normalized gate passed.

Two inlined memcmp operations exchange the ring-buffer and literal operands. Only equality is consumed, so the sign reversal is irrelevant. The original literal bytes are :IMPROVISE and :SHOWCAPACITY. Their two complementary compiler-literal relocation positions remain unresolved, not silently promoted to matches.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 205 | `mov esi, 0x4bae30` | `_g_type_buf +0xa` | `0x00668d9e` |
| 239 | `mov esi, 0x4badf0` | `_g_type_buf +0x7` | `0x00668d9b` |

### `LFTrack_Update` — `LEGOLAND/logflume.c` — `0x0040c4a0`

Class **G**: Arithmetic operands exchanged. 163 instructions / 538 bytes; normalized gate passed.

The footprint and map-reference byte loads exchange AL/BL before the same byte addition. The sum and truncation are unchanged.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 79 | `mov bl, byte ptr [0x4b472c]` | `_g_mapref +0x4` | `0x007fffc8` |
| 81 | `mov al, byte ptr [0x7fffc8]` | `_g_lf_footprint +0x4` | `0x004b472c` |

### `LFPiece_TickCommon` — `LEGOLAND/logflume2.c` — `0x0040d3b0`

Class **F**: Independent copies or equal-value stores reordered. 22 instructions / 100 bytes; normalized gate passed.

A chained assignment stores 0x2034 into the same four distinct nonvolatile globals in another order. All four final values are unchanged.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 17 | `mov dword ptr [0x4cbdd8], eax` | `_g_lf_tool_d +0x0` | `0x004c74c8` |
| 18 | `mov dword ptr [0x4c2a88], eax` | `_g_lf_tool_c +0x0` | `0x004c5c90` |
| 19 | `mov dword ptr [0x4c5c90], eax` | `_g_lf_tool_b +0x0` | `0x004c2a88` |
| 20 | `mov dword ptr [0x4c74c8], eax` | `_g_lf_tool_a +0x0` | `0x004cbdd8` |

### `RenderMouseBounds` — `LEGOLAND/mapscreen.c` — `0x00456460`

Class **G**: Arithmetic operands exchanged. 105 instructions / 321 bytes; normalized gate passed.

The title offset and view.y loads exchange registers before both are subtracted from mouse y. The result is y - offset - view.y in either order; later view.y use reloads its correct address.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 12 | `mov esi, dword ptr [0x8139cc]` | `_g_ms_y_off +0x0` | `0x00667c20` |
| 14 | `mov edi, dword ptr [0x667c20]` | `_g_ms_view +0xc` | `0x008139cc` |

### `MapScreenSetScrollPos` — `LEGOLAND/mapscreen.c` — `0x004565b0`

Class **G**: Arithmetic operands exchanged. 101 instructions / 320 bytes; normalized gate passed.

The same two subtractands exchange registers in the mouse-to-map calculation and again in the scroll calculation. Both subtractions are retained; the sequential integer result is unchanged.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 11 | `mov ebx, dword ptr [0x667c20]` | `_g_ms_view +0xc` | `0x008139cc` |
| 13 | `mov edi, dword ptr [0x8139cc]` | `_g_ms_y_off +0x0` | `0x00667c20` |
| 66 | `mov ebx, dword ptr [0x667c20]` | `_g_ms_view +0xc` | `0x008139cc` |
| 67 | `mov esi, dword ptr [0x8139cc]` | `_g_ms_y_off +0x0` | `0x00667c20` |

### `PlaneRide_Create` — `LEGOLAND/mechrides.c` — `0x0043dda0`

Class **C**: Wrong same-size global references. 81 instructions / 318 bytes; normalized gate passed.

g_plane_bnv0/1/2 are annotated as aliases of the ride/on/off source pointers (0x0062fe90/94/78). The original copies them into distinct destination globals 0x0062fe84/88/8c. The current reconstruction writes the source globals back to themselves, leaving the original destination slots unpopulated by these copies. These aliases are also used by PlaneRide_Destroy; the integrator must audit both uses rather than simply relabeling shared declarations.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 34 | `mov dword ptr [0x62fe84], edx` | `_g_plane_bnv0 +0x0` | `0x0062fe90` |
| 35 | `mov dword ptr [0x62fe88], eax` | `_g_plane_bnv1 +0x0` | `0x0062fe94` |
| 36 | `mov dword ptr [0x62fe8c], ecx` | `_g_plane_bnv2 +0x0` | `0x0062fe78` |

### `Copters_Activate` — `LEGOLAND/mechrides.c` — `0x00404be0`

Class **C**: Wrong same-size global references. 229 instructions / 742 bytes; normalized gate passed.

The boarding and alighting switches select wrong globals for cases 0, 3 and 4; cases 1 and 2 agree. The original boarding/alighting jump tables at 0x00404ef8/0x00404f0c both map cases 0..4 to path addresses 0x004c112c, 0x004c1124, 0x004c1128, 0x004c1130, 0x004c1134. Source selects 0x004c1130, 0x004c1124, 0x004c1128, 0x004c1134, 0x004c112c. Physical block ordering accounts for why there are eight positional hits rather than six changed case selections. Check associations against Copters_Create as well as declaration names.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 73 | `mov eax, dword ptr [0x4c1124]` | `_g_copters_path2 +0x0` | `0x004c1130` |
| 76 | `mov ecx, dword ptr [0x4c112c]` | `_g_copters_path0 +0x0` | `0x004c1124` |
| 81 | `mov eax, dword ptr [0x4c1130]` | `_g_copters_path3 +0x0` | `0x004c1134` |
| 84 | `mov ecx, dword ptr [0x4c1134]` | `_g_copters_path4 +0x0` | `0x004c112c` |
| 145 | `mov ecx, dword ptr [0x4c1124]` | `_g_copters_path2 +0x0` | `0x004c1130` |
| 148 | `mov edx, dword ptr [0x4c112c]` | `_g_copters_path0 +0x0` | `0x004c1124` |
| 153 | `mov ecx, dword ptr [0x4c1130]` | `_g_copters_path3 +0x0` | `0x004c1134` |
| 156 | `mov edx, dword ptr [0x4c1134]` | `_g_copters_path4 +0x0` | `0x004c112c` |

### `StoreNewSaveGameToDisk` — `LEGOLAND/profiles.c` — `0x0048e870`

Class **F**: Independent copies or equal-value stores reordered. 112 instructions / 412 bytes; normalized gate passed.

Independent profile copies are scheduled in a different order. Despite six differing positions, original and source both map current+0x24 to temporary+0x28, current+0x28 to temporary+0x2c, and current+0x20 to temporary+0x20; the unaffected current+0x2c to temporary+0x30 copy also agrees.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 37 | `mov edx, dword ptr [0x80ffc4]` | `_g_cur_profile +0x28` | `0x0080ffc8` |
| 38 | `mov ecx, dword ptr [0x80ffc0]` | `_g_cur_profile +0x24` | `0x0080ffc4` |
| 40 | `mov eax, dword ptr [0x80ffc8]` | `_g_cur_profile +0x20` | `0x0080ffc0` |
| 41 | `mov dword ptr [0x7cad88], edx` | `_g_temp_profile +0x2c` | `0x007cad8c` |
| 43 | `mov dword ptr [0x7cad8c], eax` | `_g_temp_profile +0x20` | `0x007cad80` |
| 44 | `mov dword ptr [0x7cad80], ecx` | `_g_temp_profile +0x28` | `0x007cad88` |

### `GetGFXFName` — `LEGOLAND/rin.c` — `0x0044de90`

Class **D**: Equivalent switch-block order. 118 instructions / 329 bytes; normalized gate passed.

The physical case blocks for directory indices 3/4/5 are reordered, but the original jump table at 0x0044dfdc sends cases 4/5/6 to 0x0044df69/0x0044df2e/0x0044df4b, selecting indices 5/3/4. Those are exactly the source mappings. The differing addresses therefore belong to equivalent switch-block ordering, not wrong directory selection. This case was manually classified using the original table; table contents remain outside the tool's automatic checks.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 55 | `mov eax, dword ptr [0x4b81cc]` | `_g_gfx_dirs +0x14` | `0x004b81d4` |
| 66 | `mov edx, dword ptr [0x4b81d0]` | `_g_gfx_dirs +0xc` | `0x004b81cc` |
| 77 | `mov ecx, dword ptr [0x4b81d4]` | `_g_gfx_dirs +0x10` | `0x004b81d0` |

### `AnyCoasterRegionFullyInside` — `LEGOLAND/schoolcar.c` — `0x00426650`

Class **E**: Incorrect base address in declaration. 13 instructions / 41 bytes; normalized gate passed.

The g_coaster_regions annotation starts with 0x008299a0, but the same comment says its own base is 0x00829a3c and next is 0x00829a54. The original loads next at 0x00829a54 and compares with base 0x00829a3c. This is an inconsistent address declaration, not evidence that the list-walking C needs a structural rewrite.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 0 | `mov eax, dword ptr [0x829a54]` | `_g_coaster_regions +0x18` | `0x008299b8` |
| 1 | `cmp eax, 0x829a3c` | `_g_coaster_regions +0x0` | `0x008299a0` |
| 7 | `cmp eax, 0x829a3c` | `_g_coaster_regions +0x0` | `0x008299a0` |

### `SetupTrackDrawView` — `LEGOLAND/schoolcar.c` — `0x00425bd0`

Class **I**: Aggregate copy and floating-point operand order. 19 instructions / 105 bytes; normalized gate passed.

The source copies all four ViewRec fields together and stores the angle separately; original copies b/c/d, then angle, then a. Register and destination permutations preserve each final field mapping for ordinary sequential execution. The initial fld/fadd inputs are also exchanged: ordinary finite results agree, but NaN payload/exception behavior and asynchronous observation are not certified. This remains a strict address-identity mismatch, not the loss of one vector field.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 0 | `fld dword ptr [0x4b5cac]` | `_g_view_angle +0x0` | `0x0061164c` |
| 1 | `fadd dword ptr [0x61164c]` | `_g_view_wide +0x0` | `0x004b5cac` |
| 2 | `mov eax, dword ptr [0x4b5cb0]` | `_g_view_wide +0x0` | `0x004b5cac` |
| 3 | `mov ecx, dword ptr [0x4b5cb4]` | `_g_view_wide +0x4` | `0x004b5cb0` |
| 4 | `mov edx, dword ptr [0x4b5cb8]` | `_g_view_wide +0x8` | `0x004b5cb4` |
| 5 | `mov dword ptr [0x4b5ca0], eax` | `_g_view_cur +0x0` | `0x004b5c9c` |
| 7 | `mov eax, dword ptr [0x61164c]` | `_g_view_wide +0xc` | `0x004b5cb8` |
| 8 | `mov dword ptr [0x4b5ca4], ecx` | `_g_view_cur +0x4` | `0x004b5ca0` |
| 9 | `mov ecx, dword ptr [0x4b5cac]` | `_g_view_angle +0x0` | `0x0061164c` |
| 10 | `mov dword ptr [0x4b5ca8], edx` | `_g_view_cur +0x8` | `0x004b5ca4` |
| 11 | `mov dword ptr [0x611648], eax` | `_g_view_cur +0xc` | `0x004b5ca8` |
| 12 | `mov dword ptr [0x4b5c9c], ecx` | `_g_view_angle_cur +0x0` | `0x00611648` |

### `Coaster3D_EndFrame` — `LEGOLAND/schoolcar.c` — `0x00423140`

Class **E**: Incorrect base address in declaration. 61 instructions / 191 bytes; normalized gate passed.

The same inconsistent g_coaster_regions base affects the end-frame clip-list walk. Original base/next are 0x00829a3c/0x00829a54; annotation-based resolution gives 0x008299a0/0x008299b8. Shared root cause with AnyCoasterRegionFullyInside.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 21 | `mov esi, dword ptr [0x829a54]` | `_g_coaster_regions +0x18` | `0x008299b8` |
| 22 | `cmp esi, 0x829a3c` | `_g_coaster_regions +0x0` | `0x008299a0` |
| 31 | `cmp esi, 0x829a3c` | `_g_coaster_regions +0x0` | `0x008299a0` |

### `MapIconInput` — `LEGOLAND/screens3.c` — `0x00475080`

Class **F**: Independent copies or equal-value stores reordered. 37 instructions / 147 bytes; normalized gate passed.

The two affected globals both receive EBX=1, in another order. The old game mode was already loaded for the saved-mode store; the permutation does not change that saved value.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 18 | `mov dword ptr [0x8119b4], ebx` | `_g_8119bc +0x0` | `0x008119bc` |
| 20 | `mov dword ptr [0x8119bc], ebx` | `_g_game_mode +0x0` | `0x008119b4` |

### `SoftBlitSprite` — `LEGOLAND/softblit.c` — `0x00464ee0`

Class **G**: Arithmetic operands exchanged. 242 instructions / 742 bytes; normalized gate passed.

The surface pitch and mouse y operands exchange positions in one signed integer multiply. Their product and the resulting mouse-pixel address are unchanged.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 45 | `mov eax, dword ptr [0x6680ac]` | `_g_mouse_point +0x4` | `0x00813a48` |
| 46 | `imul eax, dword ptr [0x813a48]` | `_g_lock +0x10` | `0x006680ac` |

### `SoftPrint_Clear` — `LEGOLAND/text.c` — `0x004651d0`

Class **F**: Independent copies or equal-value stores reordered. 33 instructions / 105 bytes; normalized gate passed.

Width and height loads exchange registers and their matching destination stores also exchange. Each cached dimension still receives the correct source dimension before the assembly clear loop.

| Index | Original operand | Our COFF symbol + addend | Our target |
| ---: | --- | --- | --- |
| 7 | `mov ecx, dword ptr [0x6680a4]` | `_g_ddsd_width +0x0` | `0x006680a8` |
| 9 | `mov eax, dword ptr [0x6680a8]` | `_g_ddsd_height +0x0` | `0x006680a4` |
| 10 | `mov dword ptr [0x7fea14], ecx` | `_g_sp_width +0x0` | `0x007fea1c` |
| 11 | `mov dword ptr [0x7fea1c], eax` | `_g_sp_height +0x0` | `0x007fea14` |

## Unresolved positions

Unresolved positions are not false-positive mismatches and are not certified
matches. The full machine-readable ledger can be reproduced with `--json`;
all 1,255 are also printed individually by the sweep.

| Reason | Positions | Worked example |
| --- | ---: | --- |
| jump table/local code symbol | 103 | `Road_Restitch` i62: original jump-table target 0x0041391c; ours `$L667` has no independent VA. |
| string literal | 434 | `PlayNarrationFile` i6: original 0x004bfeec; ours `??_C@_07BBH@speech?2?$AA@` has no VA binding. |
| symbol without address annotation | 526 | `Load_FXList` i13: original call 0x0049e573; local `_sprintf` has no address annotation/system-header mapping. |
| floating-point literal | 166 | `CalcMoveLine` i18: original 0x004ab538; ours `__real@8@40078000000000000000` has no VA binding. |
| conflicting address annotations: 0x004bcec0, 0x004bcec4 | 2 | `SuggestNextMove` i9/i12: one g_suggest_target declarator lists 0x004bcec0 and 0x004bcec4; parser refuses to guess the intended base. |
| import slot without address annotation | 24 | `CoasterModel_SetDirectory` i5: original IAT slot 0x004ab268; ours `__imp__SetCurrentDirectoryA@4` lacks explicit IAT metadata. |

## Stage verification and review

1. **Baseline/environment — complete.** Read the scope, contract and relevant
   matching docs; fetched main and created an isolated `scope/L` worktree at
   `3601892`. Existing `audit.py` passed all 26 exact bodies in `coaster9.c`.
   Original executable hash matches the repository's expected target.
2. **Tool/regressions — complete.** `python3 tools/relocs.py --self-test`
   passes synthetic COFF/addend/operand tests and source-annotation tests;
   `python3 -m py_compile tools/relocs.py` passes. Cases include nonzero
   function offsets, auxiliary symbol records, decorated names, negative
   addends, two operands in one instruction, wrong globals/callees, unresolved
   literals, conflicting metadata, IAT separation, static/export collisions,
   macro continuations, naked markers and next-line comment association.
   Parser marker bindings agree with every `audit.annotated` entry in the
   entire source tree (exact and WIP).
3. **Historical cases — complete.** A temporary reversed-store
   `NewMechanicOrder` still passes normalized matching but produces exactly
   two relocation hits, at i8 and i10. The committed body resolves all nine
   positions and exits 0. A temporary whole-record car-view copy produces
   ten strict hits; the committed `Coaster3D_SetCarClipDepth` has zero hits,
   16 resolved positions and two unresolved float literals. These are positive
   detection tests; a different copy schedule alone is not proof of a behavior
   bug. All mutation fixtures stayed under `/tmp/sl_`.
4. **Failure continuation — complete.** A temporary three-file sweep with
   valid/invalid/valid C produces checked/skipped/checked, reports exactly one
   compile failure and exits 2. No stale object is reused after failure.
5. **Full sweep/classification — complete.** Final run checks all 2,355 bodies
   and reports the counts above. Independent review examined COFF handling,
   source-comment grammar and the first eight hit functions; the remaining
   functions were traced through their source and original operands. Parser
   defects found in the first pass were fixed before the complete rerun.
   Final review also decoded the original GetGFXFName and Copters jump
   tables: physical block order is not case order. GetGFXFName retains the
   correct case mapping; Copters cases 0/3/4 differ in both switches.
   The report ledger is checked programmatically against every mismatch row
   in the final JSON, including per-class and unresolved totals.

The remaining limitation is explicit: **completion of Scope L does not mean
all 2,355 bodies have verified relocation identities**, nor that 77 differences
are 77 gameplay bugs. It delivers the new tool, complete sweep and classified
review list. The integrator can now correct the seven flagged functions/root
causes at the required quiet-tree gate, and decide whether the thirteen order-only
cases need stricter binary matching. No such source fixes are part of this lane.
