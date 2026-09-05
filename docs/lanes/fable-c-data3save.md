# fable-c lane `data3save` — `LEGOLAND/data3.c` + `LEGOLAND/savegame2.c`

Locale text/textures, then the save-load helpers (SCOPE §data3.c, §savegame2.c).
**9 of 9 exact, both files `audit.py` PASS, `/W3` clean.** Seven of the nine
matched on the first compile; two took one and four extra variants.

| address | name | insns | bytes | audit | marker |
| --- | --- | --- | --- | --- | --- |
| 0x004402d0 | `LoadTextFile` | 47 | 119 | [OK] 0 | `// FUNCTION: LEGOLAND 0x004402d0` |
| 0x0043f990 | `LoadLocSet` | 49 | 123 | [OK] 0 | `// FUNCTION: LEGOLAND 0x0043f990` |
| 0x00443720 | `LoadLocTextures` | 58 | 173 | [OK] 0 | `// FUNCTION: LEGOLAND 0x00443720` |
| 0x004428f0 | `LookupTextureName` | 60 | 137 | [OK] 0 | `// FUNCTION: LEGOLAND 0x004428f0` |
| 0x00450a80 | `SaveBuildSlots` (was `SaveBlock11`) | 46 | 142 | [OK] 0 | `// FUNCTION: LEGOLAND 0x00450a80` |
| 0x0046c680 | `LoadScriptString` | 50 | 116 | [OK] 0 | `// FUNCTION: LEGOLAND 0x0046c680` |
| 0x00482860 | `SavePathRects` | 54 | 136 | [OK] 0 | `// FUNCTION: LEGOLAND 0x00482860` |
| 0x00482920 | `LoadPathRects` | 57 | 146 | [OK] 0 | `// FUNCTION: LEGOLAND 0x00482920` |
| 0x004424e0 | `RecolourModelParts` | 61 | 156 | [OK] 0 | `// FUNCTION: LEGOLAND 0x004424e0` |

Every body ends in a real `ret` and none is recursive, so all nine are
promotable. No WIPs, no residuals, no ESCAPES.

---

## Levers (with evidence)

- **An uninitialised local read on a merge path is a SINGLE-RETURN shape, and
  splitting the return kills it.** `LoadTextFile`'s failure epilogue is
  `mov eax,[esp+0xc]` — the never-written home of the result pointer. Written
  with the failure's own `return text;` (`if (f) { ...; return text; } return
  text;`) VC6 sinks two of the three `push`es into the guarded block (the
  recorded "a push sinks past a leading guard when the guarded block ends in
  its own `return K`" rule fires on a `return <var>` too) and the body comes
  out 45 instructions with the frame read as `[esp+4]`: 31/45. **One** `return
  text;` after the `if` restores all three pushes to the prologue AND makes
  VC6 tail-duplicate the epilogue itself, reading the uninitialised home at
  `[esp+0xc]` — 47/47 exactly. `if (f == 0) return text;` (failure inline) is
  the third shape and is wrong in a different way. **Do not write the second
  return: let VC6 duplicate the epilogue.**
- **Two uninitialised locals, one per guard, are how a function gets two
  different frame homes — and this is reachable, not a compiler accident.**
  `LookupTextureName` returns one of two pointers, each assigned only inside
  its own guard. Written as `list = ...` (reusing the parameter, the natural
  reading) VC6 promotes the parameter to `esi`, root-copies it at entry and
  needs a THIRD callee-saved register: `push ebx` appears, `cmp
  dword ptr [esp+..],1` replaces the original's load, and the two tail arms
  push registers instead of memory — 60 instructions, ~20 mismatches. Written
  as two locals (`face`, `chest`) that are each left undefined on one path,
  VC6 homes `chest` in the DEAD `list` argument slot and `face` in the `index`
  argument slot — the original's `mov [esp+0xc],eax` store and its two
  `mov eax,[esp+...]` tail loads — and everything through index 39 matches.
  **`face`'s home is the `index` slot even though `index` is still live**,
  which is why the shipped function calls `SkipStrings((char*)index, index)`
  when the list pointer is null. Both compilers agree on that slot, so the
  bug reproduces byte for byte.
- **A one-case `switch` emits `mov eax,<arg> / dec eax / je`; `if (x == 1)`
  emits `cmp dword ptr [mem],1 / je`.** That single instruction was the whole
  residual of `LookupTextureName` (59 vs 60). `switch (kind) { case 1: ... }`
  with a trailing return and `switch (kind) { case 1: ... default: ... }` are
  byte-identical; `if (kind - 1 != 0)` and `kind--; if (kind != 0)` both give
  `mov/dec/test/je` — one instruction too many — and also re-rank the tail's
  scratch registers. Extends the recorded two-case `dec/je/dec/jne` entry to
  the one-case form: **a lone `dec` before a `je` on a parameter is a
  `switch`, not an `if`.**
- **The for-increment order of two explicit lockstep cursors decides the
  latch's emission order; the initialiser order is inert.** `RecolourModelParts`
  ends `add esi,0x24 / inc edi`. A subscript walk (`for (i = 0; i < n; i++)`
  with `parts[i]`) gives the reverse — `inc edi / add esi,0x24` — and was the
  only mismatch in an otherwise exact 61-instruction body. With the pointer
  named, `for (i = 0, p = parts; i < n; p++, i++)` and
  `for (p = parts, i = 0; i < n; p++, i++)` are both exact and
  `..., i++, p++)` is both wrong. **Sweep the increment order, not the
  declaration order.** This is the reachable case of the recorded
  "IV emission order in a latch" negative (`BsWater_SetTile`), by the same
  route `ZBuffer_RunCommand` used: it is the source order of the two updates.
- **A subscript walk over a global array gives a SIGNED `cmp <cursor>,<end> /
  jl`.** `SaveBuildSlots`'s two loops compare a strength-reduced cursor
  against `0x6670f8` with `jl`, which reads like a pointer walk but is not —
  a pointer walk compares unsigned (`jb`). `for (i = 0; i < 256; i++)` over
  `g_build_slots[i]` produces it exactly, twice, first try. Confirms the
  recorded "counted `for` gives `cmp/jl`, pointer walk gives `jb`" from the
  read side, on an array whose bound is a link-time constant.
- **A 12-byte struct assignment is `mov ecx,<src>` plus three register-move
  pairs.** `rec = g_build_slots[i];` gives the original's
  `mov ecx,esi / mov edx,[ecx] / mov [esp+0xc],edx / ...` — the source-address
  copy into `ecx` is part of the struct-assignment lowering, not evidence of a
  second pointer variable. (Same family as the recorded "a 16-byte copy is
  four register moves".)
- **An address-taken counter is incremented THROUGH MEMORY.**
  `SavePathRects`'s counting loop is `mov edx,[esp] / inc edx / mov [esp],edx`
  because `&n` is later passed to `SaveGameWrite`; `SaveBuildSlots`'s counter,
  also address-taken, still lives in `ecx` across its loop — the difference is
  that `SavePathRects`'s loop body contains the only other use. Read the
  recorded "an address-taken counter turns strength reduction off" as:
  address-taken forces the home store, and whether the loop also reloads
  depends on what else is live.
- **`while (n-- != 0)` on an UNSIGNED memory-homed count is
  `mov/mov/dec/test/mov/je`, with the whole test duplicated in the latch.**
  `LoadPathRects` — exact first try; the count is memory-homed because `&n`
  went to `SaveGameRead`, so the pre-decrement value has to be materialised in
  a second register (`mov ecx,eax`) before the store.
- **Repeated `if (!Write(...)) return 0;` arms cross-jump BACKWARDS into the
  first inline `return 0`.** `SavePathRects`/`LoadPathRects`: the first check's
  failure block is laid down inline right after it, and the three later checks
  inside the loop all `je` back to it. No construct needed — write the guards
  naturally and VC6 merges them.

## Mechanics recovered

- **`.\3ddata\new\<dir>\<file>` is the one path format** (0x004b7b10) for the
  whole 3D-person data set: `LoadTextFile`, `LoadLocSet` and `LoadAnim3D` all
  build with it, and `LoadLocTextures` builds its textures with a second
  format `"%s\\%s%04d.BMP"` (0x004b7d58) rooted at the same directory.
- **`LocSet` (the ".loc" file) layout, from three functions:** `+0x00` texture
  count, `+0x04` the model context `InitMan` writes back, `+0x0c` the texture
  file stem, `+0x2c`/`+0x30` two FILE-RELATIVE offsets that 0x0043f970 turns
  into absolute pointers as the last step of the load. `data2.c`'s `LocSet`
  (pad0/ctx) is the same struct seen from the caller; `+0x00` is the count.
- **The packed texture-name list** that `LoadTextFile` returns is
  `<title>\0 <u32 n> <face name>\0 ... \0 ""\0 <title>\0 <u32> <chest name>\0
  ...` — a title string and a 4-byte count per half, the face half terminated
  by an empty string. `LookupTextureName(list, kind, index)` returns the
  `index`'th entry of the face half (`kind != 1`) or the chest half
  (`kind == 1`), which is exactly what `blokeai.c` describes from the caller
  side.
- **Texture registration:** `LoadLocTextures` advances a global texture-id
  cursor (0x00665e8c — the value `GetModelContext` returns) once per texture
  **even when the texture fails to load**, so ids stay in step with the set's
  own numbering; it records each texture's pixel size in `g_texsize`
  (0x0081c0c0, the table `anim2.c` reads) and drops the source image again.
- **BLK 11 of the .sav is the "under construction" table**, 256 x 12-byte
  `BuildSlot` (0x006664f8, `buildtick.c`'s table): a count of occupied slots
  then one record each with the live object pointer replaced by its `g_elist`
  index (via the object's element at `+0xc4`). The same 0xc00 region is ALSO
  written raw as part of BLK 9 — BLK 11 is the pointer-bearing view of it.
- **BLK 10 is the path-square list** (0x0066b44c, `pathsq.c`'s `PathSquare`):
  a count, then per square the 0x14-byte `Rect` (its `next` link included, so
  a live pointer goes to disk and comes back), then `+0x1c` and `+0x20`. The
  loader frees the old list through `sub_4828f0` and pushes each record on the
  head, so **the list comes back REVERSED**.
- **Script-string framing, loader half:** `u32 len` then `len` bytes; the
  terminating NUL is added in memory, not stored. `len == -1` is the "no
  string" marker and returns 0 *without* bumping `g_script_errors` — that is
  what lets `savechunks2.c` tell an absent name from a failed read.

## Callees named for the first time

| address | name given | what it is |
| --- | --- | --- |
| 0x0043f970 | `FixUpLocSetPointers` | 8 instructions: adds the block base to the `LocSet` offsets at `+0x2c`/`+0x30` |
| 0x004436d0 | `LoadTextureImage` | `CreateSourceImage(path, fmt)` + the 0x004434d0 conversion, `KillImage`s and returns 0 on failure |
| 0x00488670 | `RegisterTextureImage` | mallocs a 0x2c-byte texture record, fills it from the image (0x004437d0) and files it in `g_textures[slot]` (0x00798190, 256 slots) |
| 0x004428c0 | `SkipStrings` | steps `n` NUL-terminated strings forward |
| 0x00665e8c | `g_model_ctx` | the running texture-id cursor `GetModelContext` (0x00443710) returns |

`SaveBlock11` (0x00450a80) is **renamed `SaveBuildSlots`**, following
`savechunks.c`'s precedent (`SaveBlock5..8` are `SaveGardeners`,
`SaveMechanics`, `SaveGardenerOrders`, `SaveMechanicOrders`). `savegame.c`'s
`extern void SaveBlock11(void);` is left untouched.

## Original bugs reproduced

1. **`LoadTextFile` returns an uninitialised pointer** when the file cannot be
   opened (`mov eax,[esp+0xc]` off a never-written frame slot). `data2.c`'s
   `InitMan` stores that straight into `g_texnames_boy`/`g_texnames_girl`.
2. **`LoadTextFile` and `LoadLocSet` both leak the RES handle** when the
   allocation fails — `RES_CloseFile` sits inside the `if (buffer)`.
3. **`LookupTextureName` with a null list** calls
   `SkipStrings((char*)index, index)` — the uninitialised `face` pointer is
   homed in the `index` argument slot.
4. **`LookupTextureName` with a zero count** leaves `chest` unset; its home is
   the dead `list` argument slot, which still holds the caller's pointer, so
   the chest lookup silently walks from the head of the file.
5. **`LoadScriptString` never checks its allocation** — a failed malloc reads
   into a null pointer.
6. **`RecolourModelParts` never reads `k2`.** The caller (`savechunks.c`)
   builds two key/replacement pairs; what shipped tests `k1` for every part
   and chooses the replacement by the part's POSITION (first half of the parts
   gets `r2`, second half `r1`). With the caller's arguments only the 0x56
   grey is ever matched and the black `key2` does nothing.
7. **`RecolourModelParts` reads FOUR bytes out of a three-byte `Colour3`**
   (`mov eax,[ebp]`) before handing the address to `MakeShadedColour`; the top
   byte is whatever follows the key in the caller's frame. Harmless —
   `MakeShadedColour` reads three.
8. **`SaveBuildSlots` ignores both `SaveGameWrite` results**, unlike the
   neighbouring block writers.
9. **`SavePathRects` writes the `Rect`'s live `next` pointer to disk** and
   `LoadPathRects` reads it straight back into the record (it is then dead —
   the list link is `+0x00`, not `rect.next`).

## Extern-type divergences

- `savechunks.c` declares `RecolourModelParts(..., void* inst, int n)`; this
  file defines it with `ModelPart* parts`. ABI-identical under `__cdecl`, left
  divergent on purpose.
- `savegame.c` declares `FindeIneList(void* pval)`; used here with `&elem`
  where `elem` is `int`. Kept as `void*` to match.
- `data2.c` declares `LocSet` as `{ int pad0; void* ctx; }` — the same struct;
  `+0x00` is the texture count, which `data2.c` never reads.
- `screens3.c`/`profiles.c` name 0x004828f0 `sub_4828f0`; kept, one name per
  address.

## Negatives (recorded so they are not re-derived)

- `LoadTextFile` with the failure written as an early `if (f == 0) return
  text;`: the failure block lands INLINE after the guard, not at the end.
  Three shapes exist for this function and only the single-return one matches.
- `LookupTextureName` with `list` reassigned instead of a second local: 60
  instructions, three callee-saved pushes, wrong everywhere from index 0. The
  register pressure — not the arithmetic — is the whole difference.
- `if (kind - 1 != 0)` and `kind--; if (kind != 0)` for the `dec eax / je`:
  both emit an extra `test` and shift the tail registers. Only the `switch`
  reaches it.
