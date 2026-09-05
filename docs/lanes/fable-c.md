# fable-c — parallel session notes (2026-09-05)

Findings from the five files docs/SCOPE_FABLE_C.md assigned to this session, for folding into docs/DECOMP.md at integration. Per-lane originals beside this file carry the evidence.

| file | exact | WIP | notes |
| --- | --- | --- | --- |
| `LEGOLAND/data3.c` | 4 | 0 | `fable-c-data3save.md` |
| `LEGOLAND/savegame2.c` | 5 | 0 | `fable-c-data3save.md` |
| `LEGOLAND/mapscreen4.c` | 5 | 0 | `fable-c-mapscreen4.md` |
| `LEGOLAND/sysmisc2.c` | 5 | 0 | `fable-c-sysmisc2.md` |
| `LEGOLAND/workorder4.c` | 7 | 0 | `fable-c-workorder4.md` |

---

# data3save

## fable-c lane `data3save` — `LEGOLAND/data3.c` + `LEGOLAND/savegame2.c`

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

---

# mapscreen4

## Lane `fable-c` / `mapscreen4.c` — map screens and input

**Result: 5 of 5 exact (264 instructions), `audit.py` PASS, `/W3` clean.**
All five end in a real `ret`, none is recursive, none needed the truncated-extent
(ESCAPES) caveat, and none needed a `volatile` barrier or any other artificial
construct — four of the five matched on the FIRST draft.

| address | name | insns | pct | audit | marker committed |
| --- | --- | --- | --- | --- | --- |
| 0x00490350 | `InitScreen8` | 45 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00490350` |
| 0x00490be0 | `ReportHintInput` | 46 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00490be0` |
| 0x00452390 | `BeginMapClick` | 53 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00452390` |
| 0x004910f0 | `PrintScreenMode7` | 59 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x004910f0` |
| 0x00451f70 | `ScrollFromKeys` | 61 | 100% | `[OK]` | `// FUNCTION: LEGOLAND 0x00451f70` |

Instruction/byte lengths all agree exactly (187/137/206/194/179 bytes).

---

## Levers (evidence attached)

- **A three-term `for` increment (`i++, line++, y += 0x18`) is what puts the
  TRIP COUNTER first in the loop latch.** `PrintScreenMode7` walks fourteen
  report lines with four induction variables — trip counter, line index, the
  strength-reduced `&g_rep_lines[line]` cursor, and the y coordinate. With the
  two data steps as statements at the END of the body and only `i++` in the
  `for` increment, VC6 emits the latch as `add cursor,4 / add y,0x18 / inc i /
  cmp i,0xe` and rotates the callee-saved ranking one position (esi/edi/ebx all
  shifted): **50 of 59**. Moving both data steps into the `for` increment
  clause, after `i++`, gives the original's `inc i / inc line / add cursor,4 /
  add y,0x18 / cmp i,0xe` and fixes every register with it: **59/59**. This is
  the reachable case of the recorded "induction-variable emission order in a
  loop latch" question — the comma operator in the increment clause IS the
  ordering handle, and it is worth the whole register allocation as well as the
  latch order. Sweep it before accepting a latch-order residual as a floor.
- **A counted `for` whose bound is a literal keeps its test in the latch and
  gives the SECOND condition the loop head, with no peel.** `for (i = 0;
  i < 14; ...) { if (line > g_rep_line_count) break; ... }` is the exact source
  of a loop whose head is `cmp line,count / jg exit` and whose latch is
  `cmp i,0xe / jl head`: VC6 proves the first `i < 14` and deletes it, so there
  is no peeled copy to double as an enclosing `if`. Read this against the
  recorded "a `while` loop's condition is whichever test VC6 leaves in the
  LATCH" — when the latch test is a literal-bounded counter and there is NO
  peel, the loop was a counted `for` and the other test was a `break`.
- **Four `__cdecl` calls sharing ONE `add esp,0x40` need nothing but landing
  their results in globals/locals** — confirmed at a sixth site. `InitScreen8`
  issues `LoadSprite`, `LoadSpriteIcon`, `GetString`, `LoadSpriteIcon`,
  `GetString`, `LoadSprite` (8+20+4+20+4+8 = 0x40) and VC6 defers every
  clean-up to one `add esp,0x40` before the epilogue. No source construct
  required; the screen-init shape from `mapscreen2.c`/`mapscreen3.c` transfers
  verbatim.
- **The `InitScreen*` shape is genuinely one source template.** `InitScreen8`
  matched first try by writing `mapscreen3.c`'s `InitScreen9` body with the
  certificate screen's data: backdrop `LoadSprite`, then per icon
  `help_id = K; help = GetString(K); flags |= 0x6002; input = Handler;` in that
  order. The only structural difference is that `InitScreen8` publishes NEITHER
  handler into `g_icon_handler1`/`g_icon_handler2` (see "mechanics" below).
- **A byte-typed constant hoisted into a callee-saved register (`mov bl,4`)
  needs no construct.** `ScrollFromKeys` tests bit 2 of four different button
  state words; VC6 hoists the literal 4 into `bl` (ebx is pushed anyway) and
  emits `test byte ptr [mem],bl` at each site. Writing four plain
  `if (g_input.<btn>.state & 4)` tests reproduces it exactly. Same family as
  the recorded zero-web rule, with a non-zero constant and a byte register:
  the push is what makes it possible, and four uses is over the threshold.
- **`-(unsigned short field) * 2` is `xor/mov16/neg/shl`, and the `shl` here is
  NOT the "`x*2` must be `x+x`" case.** `-g_game->scroll_step * 2` compiles to
  `xor eax,eax / mov ax,[ecx+0x24] / neg eax / shl eax,1` — the recorded rule
  that a `*2` lowers to `lea`/`add` applies to a value that is already in a
  register and needs a fresh destination; on a freshly zero-extended value
  being pushed as an argument VC6 shifts in place. Both the negated and the
  plain arm of `ScrollFromKeys` came out first try with the plain `* 2`
  spelling.
- **A 16-byte rect copied out of a class record is FOUR field assignments, not
  a struct assignment — read it off the load order.** `BeginMapClick` loads
  `+0x3c, +0x44, +0x40, +0x48` and stores `left, right, top, bottom`: the
  non-sequential order is VC6 scheduling the two operands of the following
  subtractions (`right-left`, `bottom-top`) early. Writing the four assignments
  in the source order `left, right, top, bottom` — i.e. each subtraction's
  operands adjacent — is exact; it also keeps `left`/`top` alive in the two
  callee-saved registers for the later `sq.x + left` / `sq.y + top` sums,
  because the reads of `g_drag_rect.left` afterwards forward from the stores.
- **`x &= ~1` on an `int` in a register narrows to `and al,0xfe`** the same way
  the recorded `|= 0x100` narrows to a byte OR. The recorded entry says
  `&= ~` does NOT narrow; that was measured on a read-modify-write straight to
  memory. When the value is already loaded (here `g_input.btn0.state` is loaded
  for the guard `& 1` and stored back), VC6 uses the byte form for both
  `|= 0x1000` (`or ch,0x10`) and `&= ~1` (`and al,0xfe`). Plain source, no
  masks or casts.
- **A `do { } while (strlen(s) == 0);` is the intrinsic `repne scasb` with the
  loop's back-edge INTO the counter update.** `#pragma intrinsic(strlen)` plus
  `or ecx,-1 / xor eax,eax / repne scasb / not ecx / dec ecx / je <top>` — the
  `dec ecx` doubles as the `== 0` test, so no separate compare. Matched first
  try.

## Mechanics recovered

- **The certificate screen deliberately has no default action.** Every other
  front-end screen publishes an icon handler into `g_icon_handler1` (0x006687bc,
  Return) and/or `g_icon_handler2` (0x006687c0, Escape) — `InitScreen7` sets
  both, `InitScreen9` sets `handler2` and explicitly CLEARS `handler1`.
  `InitScreen8` writes neither, so the certificate screen inherits whatever the
  previous screen bound. That is a third distinct pattern in the family and is
  visible only as an absence, so it is recorded here.
- **The drag block below `g_input`.** Three words immediately BELOW the
  0x00813a40 game-input block are the map-drag latch, written only by
  `BeginMapClick`:
  - `0x00813a34` `g_drag_sq` — the packed `{u8 x, u8 y}` base square, copied as
    a WORD out of `g_sel_bpos` (0x00667c54).
  - `0x00813a38` / `0x00813a3c` `g_drag_step_x` / `g_drag_step_y` — the selected
    class's footprint size in map refs (`right-left+1`, `bottom-top+1`). Their
    only other reader is the drag-place walker at 0x00457f64, which strides a
    rectangular region by exactly these, so a multi-cell class drags out whole
    footprints rather than single cells.
  - `0x00813af0..0x00813afc` `g_drag_rect` — a copy of the class footprint rect
    `ObjDef+0x3c` (`objmap2.c`'s `ObjDef.rect`) WITHOUT its list link; 16 bytes,
    not `legoland.h`'s 20-byte `Rect`.
  With no class selected (`EditMode != 2` or `g_sel_def == 0`) the drag origin
  `g_input.f34/f38` is simply the map ref under the cursor; with one, it is
  biased by the class corner so the drag anchors on the object's own base
  square.
- **`g_input.flags` bit 0x1000 is set here, and the press is consumed.** The
  tail of `BeginMapClick` converts a pending press on button 0
  (`g_input.btn0.state & 1`) into the "drag in progress" flag and clears bit 0,
  so `ReadGameButtons`' click edge fires exactly once per drag.
- **`g_rep_page` is a 1-based LINE CURSOR, not a page number.** `InitScreen7`
  sets it to 1 and the Next/Previous handlers step it by 14 (0x00490b72 and 0x00490bc7); `PrintScreenMode7` prints `g_rep_lines[g_rep_page]` onwards.
  Line 0 is the heading (big font, box (0xa,0x45)-(0x1d6,0x6c)) and is never
  body text; body lines are 0x18 apart from y = 0x6b, in a 0x1cc-wide box.
- **The hint bubble is time-limited, not click-limited.** `g_rep_hint_shown`
  (0x00798888) is not a boolean: `ReportHintInput` loads it with
  `SoundTimeMS() + 0x1f40` (8000 ms) and `PrintScreenMode7` zeroes it once the
  clock passes it, then draws the bubble anchored on
  (0x1db, 5)-(0x280, 0x78) through `BubbleHelp` in mode 2.
- **`ReportHintInput` runs on hover as well as click.** `SetIconSprite(icon,
  g_rep_hint2)` is unconditional, before the event test, so the icon lights on
  any input event and only event bit 1 advances the hint.
- **The keyboard scroll is isometric-aware.** `ScrollFromKeys` steps the X axis
  by `2 * g_game->scroll_step` and the Y axis by `scroll_step` — the map is
  isometric, so the +0x24 field is a half-tile. Left/right and up/down are two
  independent if/else pairs, so a diagonal scrolls both axes but two opposed
  keys resolve to the first of the pair. It returns 1 whenever any of the four
  was held (a fifth, separate short-circuit `||` chain, not a flag accumulated
  in the arms), which is what suppresses the mouse edge scroll for that tick.

## Callees named for the first time

| address | name | evidence |
| --- | --- | --- |
| 0x004902c0 | `CertGoBackInput` | installed on `GoBack_On_Certificate.lls`; plays the UI click, calls 0x00490270, sets `g_screen_mode = 1` and clears `g_cert_active` |
| 0x00490300 | `CertPrintInput` | installed on `Print_On_Certificate.lls`; with `g_cert_result == 0 && g_cert_saving == 0` sets `g_cert_saving = 2` |
| 0x00490a60 | `PlayReportHint` | `sprintf(buf, "%s%02d.wav", 0x007cb1e0, index + 1)` then `ResetFrontEnd` and the speech player |
| 0x00491080 | `PrintReportLine` | `(text, x, y, h, big)` — returns on a null text, builds the box `(x, y)-(x + 0x1cc, y + h)` and calls `NewPrintCent(text, 3, rc, 0)` when `big`, else the other centred printer 0x00490fa0 with font 2 |
| 0x00490ea0 | `BlinkReportPageIcons` | swaps `NextPage.lls`/`NextPageLit.lls` on the blink phase unless the mouse is inside the Next icon's rect |

Strings identified: 0x004bf654 `"CertificateScreen.lls"`, 0x004bf638
`"GoBack_On_Certificate.lls"`, 0x004bf61c `"Print_On_Certificate.lls"`,
0x004bf60c `"printinfo.lls"`, 0x004bf688 `"%s%02d.wav"`.

## Original bugs reproduced

- **`ReportHintInput`'s empty-string skip has no exhaustion guard.** The
  `do { advance } while (strlen(hint) == 0);` loop wraps the index around
  `g_rep_hint_count` and re-tests forever; a hint table whose entries are all
  empty strings hangs the game. Reproduced as written, commented at the site.
- **`PrintScreenMode7`'s hint expiry is a SIGNED compare** (`cmp eax,[mem] /
  jle`, i.e. `(int)SoundTimeMS() > g_rep_hint_shown`), so once the sound clock
  passes 2^31 ms the bubble never expires. Reproduced by declaring the clock
  `int` in this file (see below).

## Extern-type divergences (deliberate, do not align)

- **0x00499450 is declared `int SoundTimeMS(void)` here**, against
  `audiomisc.c`'s `unsigned int SoundTimeMS(void)` and `fpui.c`/`bighelp.c`'s
  `GetTicks`. `PrintScreenMode7`'s expiry test is `jle`, not `ja`, so the
  return type has to be signed in THIS translation unit; `audiomisc.c`'s use
  has no comparison, so its `unsigned` spelling is equally right there. Two
  names for one address are already precedent in the tree; the type is the
  lever.
- `SetIconSprite` (0x0046d680) is declared with this file's own local `Icon*`,
  as `misc3.c`, `fpui2.c` and `bigscreens.c` each do with theirs.
- The icon input hook is typed `char (*)(Icon*, int)` here (matching
  `mapscreen2.c`), not `mapscreen3.c`'s `char (*)(Icon*, int, int, int)`. Only
  the stored pointer is load-bearing in `InitScreen8`; `ReportHintInput`'s own
  definition needs the two-argument form because it reads argument 2.

## Negative results

- `PrintScreenMode7` with the loop's data steps as body statements (the natural
  spelling) floors at 50 of 59 at the correct byte length — `strict >> rb`, an
  allocation signature. It was NOT a floor: the three-term `for` increment
  closed all nine. One more data point for "run the store/step-order sweep
  before accepting a latch-order residual", and against reaching for a
  `volatile` first.

---

# sysmisc2

## fable-c lane notes — `LEGOLAND/sysmisc2.c`

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

---

# workorder4

## fable-c lane `workorder4.c` — point-to-point routing and path rects

New file: `LEGOLAND/workorder4.c` (seven functions, 447 instructions).
Objects `/tmp/fc_workorder4_*`.

| address | name | insns | bytes | audit | mismatch | promotable |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0045d560 | `IntersectRect4` | 47/47 | 112/112 | **[OK]** | 0 | yes |
| 0x0045ca90 | `FindPathRect` | 51/51 | 136/136 | **[OK]** | 0 | yes |
| 0x0045cd70 | `RefreshPathArea` | 56/56 | 145/145 | **[OK]** | 0 | yes |
| 0x00482240 | `AddPTPOpenNode` | 58/58 | 184/184 | **[OK]** | 0 | yes |
| 0x00482620 | `PTPVisitTile` | 60/60 | 195/195 | **[OK]** | 0 | yes |
| 0x0049cf00 | `MarkWorkersOnMap` | 62/62 | 177/177 | **[OK]** | 0 | yes |
| 0x00482330 | `PTPShortcutSteps` | 113/113 | 241/241 | **[OK]** | 0 | yes |

7 of 7 exact. `audit.py LEGOLAND/workorder4.c` ends PASS; `/W3 /O2 /Gy /Gd` is
silent. Every body ends in a real `ret`; none is recursive or a tail-jump, so
all seven carry `// FUNCTION: LEGOLAND 0x<VA>` with no parenthetical.

---

## Mechanics recovered

### The 3x3 "plaza" path rect (`FindPathRect`, `RefreshPathArea`)

A run of path tiles gets a patterned paving texture wherever a complete **3x3
block** of walkable path exists. Three previously unread parallel tables at
0x004b9558 / 0x004b957c / 0x004b95a0 (nine `int`s each, laid out back to back)
drive it:

- **0x004b9558 — the nine 3x3 masks** inside a 5x5 bit map:
  `0x01ce7000, 0x00e73800, 0x00739c00, 0x000e7380, 0x000739c0, 0x00039ce0,
  0x0000739c, 0x000039ce, 0x00001ce7`. Each is `0x1ce7` (three rows of three
  adjacent bits, five apart) shifted by 1 per column and 5 per row.
- **0x004b957c / 0x004b95a0 — the matching corner offsets**, `dx` cycling
  `-2, -1, 0` and `dy` stepping `-2, -2, -2, -1, -1, -1, 0, 0, 0`.

**0x0045c9c0 named for the first time: `ScanPathArea5x5(Pos* origin)`.** It
walks the 5x5 block of cells from `origin` (x inner), starting a mask at
`0x01000000` and shifting it right one place per cell, and ORs the mask in
when the cell is walkable path — **map flags bit 4 set AND RF bit 1 clear**.
Off-map cells are scanned as `flags = 0x40, rf = 0`, so they can never
contribute. Bit 24 is the top-left cell, bit 0 the bottom-right.

`FindPathRect(pos, rect)` scans the 5x5 centred on `pos` (i.e. from
`pos - (2,2)`), takes the FIRST of the nine placements whose mask is fully
present, and reports it inclusively: `left = dx[i] + pos->x`,
`right = left + 2`, same for the y pair. Zero when no placement is complete.

`RefreshPathArea(pos)` is the repaint after the path under `pos` changed. It
sweeps the 5x5 neighbourhood **excluding the centre** (x outer, y inner) and,
for each cell that is currently showing a patterned tile but is no longer
covered by ANY complete 3x3, puts the plain tile back. Two more names:

- **0x0045ce30 `PathTileIsPatterned(Pos*)`** — on-map, the cell passes
  0x0045ce10, and the tile it DISPLAYS (`Cell +0x08`) differs from
  `*g_path_tile_base` (`*(int*)0x00832bf0`, read as a `short`).
- **0x0045cb90 `ResetPathTile(Pos*)`** — writes `*(short*)*g_path_tile_base`
  into `Cell +0x08`, with no bounds check at all.
- **0x0045ce10 `CellIsPath(Cell*)`** (for the record) — `rf & 1`, or
  `(flags & 0x10) && !(rf & 2)`.

The centre cell is deliberately left to the caller (`pathtile2.c` /
`pathsq.c`'s `RefreshPathSquare` do it).

### The point-to-point flood fill's visited map is 192 x 192 bits, not 256

`AddPTPOpenNode` and `PTPVisitTile` both index it as
`g_ptp_visited[(x >> 5) + y * 6]` — **SIX dwords per row, so 192 columns** —
and `0x0066a45c` (pathsq.c's `g_path_square_neighbours`) begins 0x1204 bytes
after 0x00669258, which is 192 rows of 24 bytes plus the four bytes of
alignment. bnvmove.c's note calls it "the 256x256-bit visited map"; it is
192 x 192. A map wider or taller than 192 cells would fold one row into the
next and run off the end of the array.

### `AddPTPOpenNode` vs `PTPVisitTile` — the same node push, two terrain rules

Both take `(int x, int y, PTPNode* parent)`, bounds-check against
`g_map->width/height`, consult the visited bitmap, `malloc(0x10)` and push
`{next, parent, x, y}` on the open list at 0x0066b450, then bump
`g_ptp_wave_count` and set the visited bit. They differ in exactly two places:

- **The terrain test.** `AddPTPOpenNode` (bnvmove.c's `PTPSuggestNextMove`,
  the bloke walker) rejects any cell with `rf & 2`. `PTPVisitTile`
  (workorder2.c's `FindPathLeg`, the build-site path finder) rejects it only
  when `!(flags & 0x0800)` — so a tile the RF layer calls impassable is still
  routable when map flag 0x0800 is set.
- **The allocation check.** `AddPTPOpenNode` tests the `malloc` result;
  **`PTPVisitTile` does not** and fills the node in unconditionally. Original
  bug, reproduced (`n = MemAlloc(0x10); n->next = ...` with no guard).

Neither sets a return value, although `bnvmove.c` and `workorder2.c` both
declare them `int` — see the extern note below.

### `PTPShortcutSteps` — corner cutting, corrected

`BuildPTPRoute` (workorder3.c) hands it the last four nodes of the walk-back:
`a` (the tile we stand on), then `b`, `c`, `d` outwards. The answer is how
many nodes one move may cover: 0 = step to `b`, 1 = to `c`, 2 = to `d`.

```
if (!b) return 0;
if (!c) return 0;
dx1 = b->x - a->x;  dy1 = b->y - a->y;        /* first step  */
dx2 = c->x - b->x;  dy2 = c->y - b->y;        /* second step */
if ((dx2 && dy1) || (dy2 && dx1))             /* the two steps TURN */
    return corner-cut tile a + (dx2, dy2) enterable ? 1 : 0;
if (!d) return 0;                             /* straight, and no fourth node */
dx3 = d->x - c->x;  dy3 = d->y - c->y;
if ((dx3 && dy1) || (dy3 && dx1))
    return corner-cut tile a + (dx3, dy3) enterable ? 2 : 0;
return 1;
```

Three things this corrects or adds to workorder3.c's account:

- It answers 0 when **`d`** is null too, not only `b` or `c` — a straight
  `a -> b -> c` with nothing beyond it is worth only one step.
- The "enterable" predicate is **`!(rf & 2) || (flags & 0x0800)`**, i.e.
  exactly `PTPVisitTile`'s. workorder3.c reads the second half as "blocked
  (Cell +0x1d bit 3)"; +0x0d is the high byte of the 16-bit `flags` at +0x0c,
  and the bit EXCUSES the tile rather than rejecting it.
- The third-step test compares `dx3`/`dy3` against the **FIRST** step's
  deltas (`dy1`/`dx1`), not the second's, and the tile it looks up is
  `a + (dx3, dy3)`. Reproduced as found; it is not obviously the geometry a
  reader would write.

**Neither cell fetch is bounds checked** — `g_map_rows[y][x]` straight off a
possibly off-map sum. It is safe only because every node on the chain came
from `PTPVisitTile`, which does bound its tiles, and the deltas are +-1.

### `MarkWorkersOnMap` — map flag 0x1000 is "a hired worker stands here"

Walks **the mechanic list (0x0079a8ac) FIRST, then the gardener list
(0x0079a8a8)**, and for each worker whose 24.8 world position at `Bloke +0x68`
maps on to the grid sets `Cell.flags |= 0x1000`. Nothing here ever clears the
bit; printlist.c's build-site check reads it immediately after calling this.

Note the bounds test shape: the `< 0` half is applied to the **raw** 24.8
value and the upper half to the shifted tile index
(`b->world.x >= 0 && (b->world.x >> 8) < g_map->width`), which is what keeps
the `test/jl` ahead of the `sar` in the original.

### `IntersectRect4`

Plain four-int rect intersection into `out`, non-zero when non-empty. Worth
recording only that the four edges are done **left, right, top, bottom** —
paired by min/max, not by axis — and that the emptiness test re-reads all four
back out of `out` rather than using the values it just computed.

---

## Levers, with evidence

- **A one-bit shift used twice must be a DUPLICATED EXPRESSION, not a named
  local — a named `bit` takes the FIRST callee-saved register and permutes all
  four.** `AddPTPOpenNode` and `PTPVisitTile` both compute
  `1 << (x & 0x1f)` for a visited-bitmap test and then for the set. With
  `unsigned int bit = 1 << (x & 0x1f);` every instruction, byte length and
  block is already right and **every one of the four callee-saved registers is
  wrong** (22 of 58 and 23 of 60): the named web outranks `x`, `y` and the
  word index, and VC6 hands it EBP/ESI where the original ranks it LAST.
  Written out twice, VC6 CSEs it into the low-ranked web the original has and
  both functions are exact first try. Measured inert on the named form:
  one-chain vs separate bounds guards, a named `Cell*`, `y*6 + (x>>5)` vs
  `(x>>5) + y*6`, all declaration orders, `int`/`unsigned` retypings of both
  locals, `register`, computing `bit` before `w` (worse: 27), and a free
  volatile read on `x`. **The word index `w` is the opposite** — it IS a named
  local; inlining it as well costs 28. So within one expression the two
  sub-expressions take opposite treatments, and the tell is the register
  RANKING, not the schedule.
- **A step-by-step delta order beats a "compute what the first test needs"
  order — worth 67 of 113 and an instruction.** `PTPShortcutSteps` computes
  four differences. Written `dx2, dx1, dy1, dy2` (the order the first
  condition consumes them, and the order VC6's own schedule appears to want)
  it spills `dx1` to the frame and keeps `c->x` in a callee-saved register;
  the original spills `c->x` and `c->y` and keeps all four deltas live.
  Written `dx1, dy1, dx2, dy2` — first step then second step — it is
  **113/113 exact**, and the operand order of the two sums that index the cell
  (`a->y + dy2` vs `dy2 + a->y`) is then completely inert. Where the earlier
  "declaration order sets the order of the leading `xor`s and nothing else"
  entry found initialiser order weak, COMPUTATION order of a group of
  same-shaped temporaries is strong: it decides which of them the frame gets.
- **A signed pointer end test (`jl`) on a strength-reduced table walk comes
  from a signed INDEX, not from a pointer cursor.** `FindPathRect`'s search
  compares the mask cursor against the address of the next table
  (`cmp ecx, 0x4b957c / jl`). A hand-written `for (m = tbl; m < tbl + 9; m++)`
  cursor with a parallel `i++` is byte-for-byte identical **except** that the
  pointer comparison is unsigned and emits `jb` — the single mismatch of 51.
  `for (i = 0; i < 9; i++)` with all three tables subscripted is exact: VC6
  strength-reduces the mask subscript into the pointer and the derived test
  inherits the `int`'s signedness, while the other two tables keep the index.
  This is the ELIMINATION half of the "two lockstep cursors" lever seen from
  the signedness side.
- **A bounds guard whose `< 0` half is on the RAW value and whose `< limit`
  half is on the SHIFTED value has to be written that way — VC6 will not sink
  the shift.** `MarkWorkersOnMap`'s original tests `test eax,eax / jl` on the
  24.8 coordinate and only then `sar eax,8` for the width compare. Passing
  `(w->x >> 8, w->y >> 8)` into an inlined `MapCellAt`-style helper forces
  BOTH shifts before the first test and turns the sign test into `js`
  (24 of 62). Spelling the guard out inline —
  `if (b->world.x >= 0 && (b->world.x >> 8) < g_map->width && ...)` — is
  62/62 first try. Corollary: **an inline bounds helper is not free when its
  arguments are expressions**; the argument evaluation is a scheduling
  barrier the original does not have.
- **A dead `lea` of an array element's address is not evidence of a named
  pointer.** `MarkWorkersOnMap` emits
  `or byte ptr [ecx+eax*4+0xd],0x10` and then a plainly dead
  `lea eax,[ecx+eax*4+0xc]`. Naming the pointer (`f = &cell.flags; *f |= ...`)
  and not naming it are byte-identical — the `lea` is VC6 materialising the
  subscript's address either way. Do not spend variants on it.
- **`if (out->left <= out->right && out->top <= out->bottom) return 1;
  return 0;`** is the shape behind two `jg` to ONE trailing `xor eax,eax`
  with `mov eax,1` inline before it (`IntersectRect4`, exact first try) —
  a fourth confirmation of the recorded "success inline, failures to one
  trailing `return 0`" rule.
- **A `continue`-per-guard inner loop is what shares the increment block.**
  `RefreshPathArea`'s body is three independent `if (...) continue;` guards
  and one call; that is 56/56 first try, with the reload of the y counter
  falling exactly where the original has it.

---

## Original bugs reproduced

- **`PTPVisitTile` does not check its allocation.** `malloc(0x10)` is followed
  straight by `mov [eax],edx` — a failed allocation writes through null. Its
  twin `AddPTPOpenNode`, twenty instructions earlier in the same source,
  does test it. Commented at the site, not fixed.
- **`PTPShortcutSteps` reads the map with no bounds check**, twice, from a sum
  of a node coordinate and a delta.
- **`ResetPathTile` (0x0045cb90) has no bounds check either** — noted at its
  extern, since `RefreshPathArea` can only reach it through
  `PathTileIsPatterned`, which does check.
- **`RefreshPathArea` skips only the exact centre cell**, so the eight
  cells immediately around a changed tile are re-tested even though
  `FindPathRect` has already been asked about overlapping blocks — the
  redundancy is in the original.

---

## Extern-type divergences and naming

- **`AddPTPOpenNode` and `PTPVisitTile` are defined `void` here** where
  `bnvmove.c` and `workorder2.c` declare both `int`. Neither definition sets a
  return value; an `int` definition would need a fabricated `return` and would
  not compile clean at /W3. The callers' `int` externs are left alone — under
  `__cdecl` they are ABI-identical, and the declared type is a caller-side
  lever there.
- **`MarkWorkersOnMap` is defined `void MarkWorkersOnMap(void)`** where
  `printlist.c` declares `void MarkWorkersOnMap(CellRect* r)` and passes the
  rect it is about to scan. The function reads no argument at all;
  printlist.c's comment already says "(ignores r)". Left alone.
- **`MemAlloc` (0x0049e4ff)** keeps bnvmove.c / workorder2.c's name here
  because this is their code path; `objrect.c`, `pathsq.c` and `printlist.c`
  call the same address `HeapAlloc_w`. Two names for one CRT address already
  existed in the tree; no new name was minted.
- `Rect4` is objrect.c's / workorder2.c's name for the flat 16-byte rect;
  `pathsq.c` calls the identical shape `PathRect` and declares
  `FindPathRect(Pos*, PathRect*)`. Offsets agree, names do not; nothing
  edited.
- `g_path_3x3_masks` is declared `const unsigned int[9]` and the two offset
  tables `const int[9]`. The masks must be unsigned or the `==` against the
  `unsigned int` scan result warns at /W3; the choice is codegen-inert.
- New names, no other file declares these addresses: **0x0045c9c0
  `ScanPathArea5x5`**, **0x0045ce30 `PathTileIsPatterned`**, **0x0045cb90
  `ResetPathTile`**, **0x0045ce10 `CellIsPath`** (described, not declared).
  0x00482330 `PTPShortcutSteps` keeps workorder3.c's name.
- `Bloke` is declared here with only `next` and `world` (+0x00, +0x68) of
  workers2.c's 0xb0-byte record; `Cell` and `Pos` come from `legoland.h`
  unchanged.
