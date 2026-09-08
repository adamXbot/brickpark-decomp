# Scope LL12 — unreferenced (dead) functions, 0x00434810..0x0043f4f0

Branch `scope/LL12` from `main` `7d756410`. New file **`LEGOLAND/unref4.c`**
(the only `LEGOLAND/*.c` file touched) plus this note and the brief
`docs/SCOPE_LL12_unref_jungle_planeride_loaders.md`.

**Result: 10 of 11 exact (`audit.py [OK]`), 1,314 of 1,408 instructions;
the one WIP (`JcDeco_CalcCursor`) sits at 94.7%, retired.**
The file ends `PASS`, `relocs.py` reports zero `MISMATCH`, `/W3` is clean.
The first session closed 8; the escalation of 2026-09-08 closed
`RunListPicker` and `BrowseForFile` (see "Closing the two large WIPs"
below) and retired `JcDeco_CalcCursor`.

## Status

| address | name | insns | % | audit | marker as committed |
| --- | --- | ---: | ---: | --- | --- |
| 0x00434810 | `JcDeco_Create` | 4 | 100 | `[OK]` | `// FUNCTION:` |
| 0x00434820 | `JcDeco_SelectForPlacement` | 14 | 100 | `[OK]` | `// FUNCTION:` |
| 0x00434860 | `JcDeco_Add` | 119 | 100 | `[OK]` | `// FUNCTION:` |
| 0x004349b0 | `JcDeco_CalcCursor` | 94 | 94.7 | REJECT | `// WIP-FUNCTION:` (retired) |
| 0x00434b20 | `JcDeco_DrawSelection` | 7 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043e930 | `Slider_Track` | 101 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043ea30 | `RunListPicker` | 395 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043eee0 | `PickLLIDBElement` | 173 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043f0b0 | `BrowseForFile` | 313 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043f460 | `TextEntryFieldStep` | 57 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043f4f0 | `RunTextEntryDialog` | 131 | 100 | `[OK]` | `// FUNCTION:` |

Residual for the one WIP:

- **0x004349b0 `JcDeco_CalcCursor`** — 89/94 aligned (strict 7/94), first
  diverging index 66. Instruction count, byte length, every block, every
  store order and every frame slot agree; the tail is **one scratch register
  out of phase** (the scratch cursor's `rect.top` temp lands in `ecx` where
  the original uses `eax`, and the following `origin` pair rotates `edx,eax`
  against the original's `ecx,edx`). Ruled out in the first session: all 24
  orderings of the four `g_jc_deco_rect2` stores (the best, committed, order
  fixed indices 26–40); `--`/`-= 1`/`= x - 1` spellings of the four rect
  adjustments; `.flags` before/after `.next = 0`; `origin` as a `Pos` struct
  copy vs two field stores and either field first; `(found & 8) != 0`;
  `int`/`unsigned`/`char` for `found`; `int` returns declared for
  `DefaultCursor`, `ValidateCursor`, `ResetCursorFootprint`,
  `SetCursorError`; `void*` for `ProbeRiver`'s out parameter. Ruled out by
  the escalation (eleven spellings, every one 89/94 except where noted): an
  empty `if (found) ;` block boundary after the rect copy and after the four
  adjustments (the LL10 single-use pin — inert on integer globals); a
  `Rect* rc` alias of the scratch rect and a `Cursor* c` alias of the whole
  scratch cursor; the top adjustment through a named temp, and all four
  adjustments through four named temps (the four-loads/four-ops/four-stores
  shape the original already has); the origin pair stored after the
  next-pointer (88/94); int-returning function-pointer casts at the
  `DefaultCursor`, `ResetCursorFootprint` and `ValidateCursor` call sites;
  the `found == 0` early return rewritten as an if/else around the whole
  tail. The phase is decided before the tail by something that emits no
  instruction; nothing left in this body's source moves it. **Retired.**

## Closing the two large WIPs (escalation, 2026-09-08)

Object prefix `/tmp/sll12b_`; every row is `matchfull.py` aligned/total.

### 0x0043ea30 `RunListPicker` — 392/398 → 395/395

The first session had the frame loop's abort value in `esi` but the loop
ROTATED (the `mov esi,[sel]` + `ProcessSystemEvents` test duplicated at the
latch). Reading the original header block instruction by instruction:
`or esi,-1 / call ProcessSystemEvents / test eax,eax / je exit`, latch a bare
`jmp header`, and `mov dword ptr [esp+0x10],-1` in the prologue — so `sel`
is initialised to -1 ONCE (memory) and the returned value is a SEPARATE
literal `-1` re-assigned every iteration.

| spelling | result |
| --- | --- |
| baseline: `for (;;) { ret = sel; if (!PSE()) break; ... }`, `return ret` | 392/398 (rotated) |
| `sel = -1` at the loop top, no `ret` (A) | 285/395 — loses the `mov [esp+0x10],-1` prologue store |
| `for (;;) { ret = -1; ... }` (B) | 387/401 — still rotated |
| `while (1) { ret = -1; ... }` (B1) | 383/395 — un-rotated; count exact |
| B1 + no-hit sites `sel = -1` instead of `sel = ret` (B4) | 385/395 |
| B4 + the three abort `break`s as `return ret` (V5) | 392/395 — the `mov eax,esi` epilogue moves BEFORE the click epilogue |
| V5 + `for (; k < n; k++, tp++)`, `tp++, k++`, body-end `k++; tp++;`, `while` forms (L1, L4–L7) | 392/395 — the row IV's `add esi,0x10` always follows `inc ebx` |
| V5 + a loop-carried `Row* rp` cursor (L2, L3) | 377/396 — anchored at `.left`, every displacement off |
| V5 + `items[k]` in the draw call, no `tp` (T1) | 393/395 — latch `inc ebx / add edi,4 / add esi,0x10` |
| T1 + six orderings/parenthesisations of the draw call's y, named `y0`/`rt`/`so` temps, `(unsigned)` casts, `(rows + k)->top`, `((int*)rows)[k*4+1]`, empty `if (k) ;` pins, `box`/`rows` declaration swaps (Y1–Y14, D1–D9, P1–P5, Q10–Q16) | 393/395 — all inert: the sum is flat and canonical (LEVERS SA02) |
| T1 + `Row* q = &rows[k]` before the draw, used in x and y (Q17, R3, R8, H1–H4, E1, S1–S6) | 394/395 — y fixed, but the row IV update now precedes `add edi,4` |
| T1 + first layout loop indexed `rows[i]` (W1a) | 392/395 |
| T1 + second layout loop indexed `rows[i].left/right` (W3, X1, X8) | 391/395 — flips the X destination too (diagnostic) |
| **T1 + second layout loop `for (i = 0; i < n; i++, q++) { q->left = 0; q->right = textw; }` (X5)** | **395/395** |

Levers, in the order they landed:

- **Header rematerialisation needs `while (1)` and a literal.** `for (;;)`
  rotates even with a two-statement header; `while (1)` with `ret = -1;` as
  the first statement keeps the folded constant-true test as the header and
  the bare `jmp` at the latch (LP05, now with a second witness). `ret = sel`
  (a copy of a variable known to be -1 on entry) is what produced the
  `or esi,-1` header PLUS a `mov esi,[sel]` latch copy.
- **`return ret` at the abort sites, not `break`.** With the un-rotated loop
  a `break` puts the loop-exit epilogue (`mov eax,esi`) AFTER the in-loop
  `return k` epilogue; three direct returns cross-jump into one epilogue
  that lands first, as in the original.
- **A compiler-derived IV and a user cursor do not interleave the same
  way.** With `tp` a user cursor its `add edi,4` sits either before `inc
  ebx` (body `tp++`) or after the derived row IV's `add esi,0x10`
  (`k++, tp++`); reading `items[k]` makes both IVs compiler-derived and the
  latch is `inc ebx / add edi,4 / add esi,0x10`.
- **The add DESTINATION of a flat three-term sum is decided by OTHER code in
  the function, not by the sum's spelling.** `box.top - g_scroll_offset +
  rows[k].top` in the draw call had `rows[k].top` as the `sub` destination
  in every spelling (six orders, temps, casts, pins); the original has
  `box.top`. Writing the second layout loop as an up-counting `for` over the
  row cursor (instead of `i = n; do { ... } while (--i)`) flips it, while
  writing that loop with `rows[i].left = 0` indexing flips the X sum's
  destination as well (W3). The loop that DEFINES the row fields sets the
  operand rank of the later reads (SA01/SA02's "definition point").

### 0x0043f0b0 `BrowseForFile` — 190/313 → 313/313

Frame map of the original (E = esp after the four pushes): `n` E+0x10,
`list` +0x14, `names` +0x18, `isdir` +0x1c, `icons` +0x20, `spr_file`
+0x24, `spr_folder` +0x28, `pass` +0x2c, `spr_drive` +0x30, `head` +0x34,
`drive[3]` +0x38, **`diff` +0x3c**, `fd` +0x40, `pattern` +0x158, `fname`
+0x25c, `cwd` +0x35c, `ext` +0x460, `dir` +0x560. Ours had no `diff` slot
(the `((a ^ b) >> 4) & 1` flag folded into `test dl,0x10`), `pass` at the
bottom, `head`/`spr_drive` swapped, and `fname`/`dir` swapped.

| spelling | result |
| --- | --- |
| baseline (two separate swap bodies under `if (diff) {...} else if (...)`) | 190/313, frame 0x64c |
| swap once under `diff ? (b->attrib & 0x10) != 0 : NameCompare(...) < 0` (P1) | 193/316 |
| P1 + `*(volatile int*)&spill = diff` phantom slot (P3a) / `*(volatile int*)&diff = ...` (P3b) | 213/317 / 271/316 |
| renaming `dir`→`path`/`folder`, `fname`→`name` (P4a–c) | 190/313 — inert |
| **swap once under `(diff != 0 && (b->attrib & 0x10)) \|\| (diff == 0 && NameCompare(...) < 0)` (P2)** | **288/312, frame 0x650** — `diff` read twice becomes a real local with a memory home at E+0x3c; `pass`, `head`/`spr_drive` and `fname`/`dir` all fall into place |
| P2 + `strcpy(g_browse_name, names[sel]); result = g_browse_name;` (T1) | 288/312 |
| P2 + `result = g_browse_name; strcpy(result, names[sel]);` (T2) | 311/313 |
| P2 + `node = head` / `prev = head` hoisted above the `n > 0` guard (T5a/T5b) | 290/312 |
| **T2 + T5b** | **313/313** |

Levers:

- **A one-shot flag that the original both stores and tests from the
  register is a local READ TWICE.** `diff` used in both halves of an
  `(a && b) || (!a && c)` condition gets a memory home (the "twelfth slot")
  and the `mov [esp+0x3c],ebx / je` pair; the ternary form (one read) does
  not, and a volatile phantom store puts the slot in the wrong pool.
- **The missing scalar slot was the whole frame story.** With it present the
  spill-pool ORDER (`pass` between `spr_folder` and `spr_drive`, `spr_drive`
  below `head`) and the `dir`/`fname` order followed with no further change;
  the first session's "two same-size buffers are not ordered by declaration
  order" observation stands, but their order was a consequence of the
  allocation, not an independent defect.
- **Assign the walker before the guard when the original loads it above
  the count test.** `mov eax,[head]` precedes `test ebx,ebx / jle`, so
  `prev = head;` is a statement above `if (n > 0)`, not a block-local
  initialiser inside it.
- **`result = dest; strcpy(result, src)` vs `result = strcpy(dest, src)`.**
  The original materialises `mov ebp,0x81c8e0` before the inline strcpy and
  feeds `edi` from `ebp`; copying the intrinsic's return instead keeps the
  constant in `edi` and copies it to `ebp` afterwards, and also let a
  reloaded `names` stay in `esi` across the picker call.

## What these functions are

Two independent subsystems, both dead in the shipped binary.

### The fourth JUNGLE CRUISE decoration class (0x00434810..0x00434b20)

`junglecruise.c` already matches this class's remove handler
(`JcDeco_Remove`, 0x00434b40) and `ridecb2.c` names its `ObjDef*` at
`0x0081cb64` and its record list at `0x00629c34`. The five bodies here are
the rest of its `SetCustomCallbacks` slots, so the class is now complete:

- `+a4 create` caches `elem->data` in `g_jc_deco_cls` — the same one-liner as
  `JungleCruiseWater_Create` (screencb7.c).
- the toolbar arm is byte-identical in shape to `JcMonkeyTree_SelectForPlacement`
  (screencb6.c 0x00433ce0): `g_edit_changed = 1`, `g_edit_object = cls`,
  `DefaultCursor(&g_edit_cursor)`, `g_ui_flags |= 8`,
  `SetEditCursorFootPrint(&cls->rect)`.
- `+94 draw-selection` is a plain `BasicObjectDCalcCursor` forwarder.
- **`JcDeco_Add`** allocates an 8-byte `{pos, owner, next}` record, asks
  `JungleCruise_ProbeRiver(pos->x - 3, pos->y, &owner)` which station owns
  the river square three cells to its left, pushes the record on
  `g_jc_deco`, calls `AddBasicObject`, and then **sets** map flag `0x40` on
  the three cells SIX squares to the left of the placement cell
  (`y`, `y-1`, `y+1`) — the exact mirror of `JcDeco_Remove`'s clear. It also
  zeroes `g_edit_cursor.next` (`0x008003f0`) before allocating.
- **`JcDeco_CalcCursor`** is the placement preview: the class footprint plus a
  second copy of the same box six squares to the left, chained onto the edit
  cursor's rect through a previously unnamed global `Rect` at `0x00616168`;
  the decoration may only be placed where the river two cells to the RIGHT
  has a west arm (`ProbeRiver` bit 8), and a previously unnamed scratch
  `Cursor` at `0x006283f8` (footprint `{left-5, top-1, right-1, bottom+1}`,
  flags `0x34`) is hung off `g_edit_cursor.next` to show the attach square.

Original bug reproduced (as in `JcDeco_Remove`): the bounds-checked cell
fetch returns NULL off-map and the very next instruction ORs into
`[0 + 0xc]`.

### A debug/tool UI: slider, list picker, text entry, file browser

`0x0043e930`..`0x0043f4f0` are a small self-contained widget set that nothing
in the shipped game reaches. Recovered mechanics:

- **`Slider_Track(Box* r, int lo, int hi, int value, int step)`** draws a
  vertical track (`RenderThickBox`) with a red 3-pixel marker at
  `top + h*value/range - 1` and returns the value the pointer asks for. A
  single global flag `g_slider_grabbed` (`0x0062fea4`) means only one slider
  in the program can be dragged at a time; the grab is taken while
  `g_mouse_ev2 & 4` and the pointer is inside the track, released when the
  button is. `step` is a DEAD parameter (the caller pushes `0x10`).
- **`RunListPicker(items, title, backdrop, r, overlay, icons, a7, a8,
  keep_scroll)`** is the modal scrolling list. Each row's height is
  `MeasureWrappedText(item, 2, colw)` clamped to a minimum of 8 plus a
  4-pixel gap; if the whole list is taller than the box the text column is
  narrowed by 16 pixels ONCE and the whole layout is redone to make room for
  the scrollbar (`{x+w-0x14, top, x+w-5, bottom}`). The scroll offset lives
  in another shared global, `g_scroll_offset` (`0x0062fea0`), which
  `keep_scroll == 0` resets. Selection is by pointer hit test against the
  row extents (in content coordinates, i.e. plus the scroll offset); a click
  (`g_mouse_ev2 & 2`) while no slider is grabbed returns the index; ESC-like
  `g_mouse_btn_a`/`g_mouse_btn_b` bit 0 abort with -1. The rows are drawn
  clipped to the box, from the first row whose bottom is at or below the
  scroll offset until one starts past the bottom; the selected row is a
  filled `RenderBlock` in `(0x7f,0x7f,0xef)`, the others a `RenderBox`
  outline in `(0xcf,0xcf,0xcf)`. `icons`, `a7` and `a8` are DEAD parameters.
- **`PickLLIDBElement(title, backdrop, r, mask, keep_scroll)`** builds the
  picker's string array from every LLIDB element whose `+0x08` flags carry
  `mask`, bubble-sorts it by name with `_stricmp`, and returns the chosen
  element (0 if the user backed out). It loads `happy.lls` / `poor.lls` and
  fills an icon array from flag bit 0 — and then passes that array to the
  picker's dead `icons` parameter, so the icons are never drawn.
- **`TextEntryFieldStep(box, font, buf, maxlen, plen)` /
  `RunTextEntryDialog(backdrop, r, title, buf, maxlen)`** are a one-field
  modal text prompt: a light panel with a dark blue title bar, the prompt in
  the bar, and an edit field inset `{8, 0x20, w-9, h-9}`. `GetInputChar`'s
  control codes here are **-3 = accept** (the step returns 1, ending the
  loop), **-2 = cancel** (returns 0 *without* redrawing) and
  **-1 = backspace**; anything else appends while `*plen < maxlen - 1`. The
  dialog returns the final length, and `font` is a dead parameter (the
  caller passes 0; the text is always printed in font 2).
- **`BrowseForFile(title, backdrop, r, pathspec)`** is the file browser on
  top of the picker: `_getcwd` / `_splitpath` / `_chdir` into the spec's
  directory, `sprintf("%s%s", fname, ext)` for the pattern, then
  `_findfirst` / `_findnext` into a singly-linked list of
  `{next, name, attrib}` records. The sort is a bubble sort whose comparator
  puts DIRECTORIES first — `((a->attrib ^ b->attrib) >> 4) & 1` decides
  whether the two entries differ in `_A_SUBDIR`, and only when they do NOT
  is `_stricmp` consulted. Folder/file icons come from `Folder.bmp` /
  `File.bmp` (`Drive.bmp` is loaded and killed but never used). Choosing a
  directory `_chdir`s into it and re-enumerates; choosing a file copies the
  name into the global buffer at `0x0081c8e0` and returns that pointer, after
  restoring the original working directory.

## Names and globals introduced

One name per address; none collides with an existing symbol (`grep`ed
`LEGOLAND/*.c` before choosing).

| address | name | why |
| --- | --- | --- |
| 0x00434810 | `JcDeco_Create` | `+a4` slot; caches the class like `JungleCruiseWater_Create` |
| 0x00434820 | `JcDeco_SelectForPlacement` | same shape as `JcMonkeyTree_SelectForPlacement` |
| 0x00434860 | `JcDeco_Add` | the `cb_add` counterpart of `JcDeco_Remove` |
| 0x004349b0 | `JcDeco_CalcCursor` | the `cb_calc_cursor` counterpart of `MonkeyFish_CalcCursor` |
| 0x00434b20 | `JcDeco_DrawSelection` | `+94`, forwards to `BasicObjectDCalcCursor` |
| 0x0043e930 | `Slider_Track` | draws and tracks a vertical slider |
| 0x0043ea30 | `RunListPicker` | the modal scrolling list |
| 0x0043eee0 | `PickLLIDBElement` | LLIDB front end for the picker |
| 0x0043f0b0 | `BrowseForFile` | the file browser |
| 0x0043f460 | `TextEntryFieldStep` | one keystroke + redraw of the text field |
| 0x0043f4f0 | `RunTextEntryDialog` | the modal text prompt |

Globals named for the first time:

| address | name | what |
| --- | --- | --- |
| 0x00616168 | `g_jc_deco_rect2` | second footprint `Rect`, chained onto `g_edit_cursor.rect` |
| 0x006283f8 | `g_jc_deco_cursor` | scratch preview `Cursor` for the decoration |
| 0x0062fea0 | `g_scroll_offset` | the list picker's scroll offset (shared by every picker) |
| 0x0062fea4 | `g_slider_grabbed` | "a slider is being dragged" (shared by every slider) |
| 0x00813acc | `g_mouse_btn_b` | second abort button, bit 0 |
| 0x0081c8e0 | `g_browse_name` | the name `BrowseForFile` returns |

Callees named for the first time (both belong to scope LL13; declared
`extern` here, not written):

| address | prototype | evidence |
| --- | --- | --- |
| 0x004551a0 | `int MeasureWrappedText(const char* text, int font, int width)` | called `(item, 2, colw)`; the result is clamped to `>= 8` and used as a row height |
| 0x00455220 | `void DrawWrappedText(int x, int y, const char* text, int font, int width)` | called `(x, y, item, 2, colw)` once per visible row |

CRT sites confirmed by use: `0x0049edcc _getcwd`, `0x0049ebff _chdir`,
`0x0049ec85 _splitpath`, `0x0049e9ed _findfirst`, `0x0049eab7 _findnext`,
`0x0049eb7c _findclose`, `0x004aab90 _stricmp` (`NameCompare`).

## Extern-type divergences

- `Slider_Track` is *defined* here with five parameters, the fifth dead,
  because `RunListPicker` pushes five and cleans `0x14`. No other file
  declares it.
- `RunListPicker` is defined with nine parameters of which three (`icons`,
  `a7`, `a8`) are never read; both callers push nine.
- `TextEntryFieldStep`'s second parameter (`font`) is dead; the caller
  passes 0.
- `RenderBlock` is declared `int`-returning here (as in `fpui.c`); making it
  `void` changed nothing measurable in `Slider_Track`.

## Levers learned (with evidence)

- **A value the original reads LAZILY inside a guarded block must be read
  there in the source too.** `Slider_Track`: hoisting `g_gfx_point.y` into a
  local before the hit test put `my` in `eax` (the `idiv` dividend) and cost
  the original's `mov eax,ecx` copy; reading `g_gfx_point.y` directly in the
  rect test and again inside `if (held)` reproduces the copy and the whole
  register plan. **43/100 -> 0/101.** The same rule fixed `JcDeco_Add`: a
  `MapCellAt(int x, int y)` helper evaluates both arguments eagerly, while
  the original loads `pos->y` only after the two `x` bounds tests — a
  `MapCellAtPos(Pos*)` helper (fpui2.c already has one) keeps the read lazy.
  **86/119 -> 0/119.**
- **`dst = c ? a : b;` and `if (c) dst = a; else dst = b;` are different
  codegen.** The ternary makes a value temp that takes the next scratch
  register and pushes every other live value one slot along (in
  `PickLLIDBElement`: `list` in `edi` instead of `esi`, the loop scratch in
  `edx` instead of `edi`); the if/else form assigns straight through the
  destination. **17/173 -> 0/173 on that one line.**
- **`for (i = n; i != 0; i--)` and `i = n; do { ... } while (i);` allocate
  DIFFERENT callee-saved registers to the values live around the loop.** In
  `RunListPicker`'s row-layout loop the do-while form put `items` in `edi`
  and `n` in `ebx`; the `for` form swaps them to the original's `ebx`/`edi`
  and makes VC6 move `items` into `edi` for the walk and reload `n` from its
  home. **176/396 -> 44/404.**
- **An infinite frame loop whose only exits are `break`/`return` is ROTATED,
  duplicating its leading guard; a `while (<variable>)` loop with the guard
  as a leading `break` inside is not — and neither is `while (1)` with a
  literal assignment as its first statement (RunListPicker, escalation).** `RunTextEntryDialog`:
  `for (;;) { if (!ProcessSystemEvents()) break; ...; if (done) break; }`
  emitted a second copy of the `ProcessSystemEvents` call at the latch;
  `done = 0; while (!done) { if (!ProcessSystemEvents()) break; ...; }`
  is exact (131/131), and the `done = 0` initialiser is free. `RunListPicker`,
  whose header also re-materialises a constant, needed `while (1)` with the
  literal `ret = -1;` as its first statement instead (see "Closing the two
  large WIPs").
- **Exiling a leading `return K`:** `if (c != 0) { if (c == -3) return 1;
  ... }` inlines the `mov eax,1` epilogue right after the compare, while
  `if (c != 0) { if (c != -3) { ... } else { return 1; } }` puts it at the
  end of the function where the original has it. `TextEntryFieldStep`
  49/57 -> 0/57 from that restructure plus making the backspace arm the
  `else` of `if (c != -1)`.
- **Adjacent stores to one global aggregate follow source order, but which
  operand gets `lea` and which gets a destructive `add` does not.** In
  `RunListPicker`'s scrollbar rect only `left, top, right, bottom` produces
  both the original's `lea ecx,[eax-0x14]` / `add eax,-5` pair AND its
  `top, right, left, bottom` store order; all 24 permutations were measured.
  The same brute-force settled `JcDeco_CalcCursor`'s four
  `g_jc_deco_rect2` stores (`editnext, top, bottom, left, right`, 12 -> 7
  strict).
- **A CSE'd sub-expression that feeds two derived values keeps its `lea`
  form only if it is spelled inline at both uses.** `Slider_Track`: passing
  a named `h` as `RenderThickBox`'s fourth argument let VC6 sink the
  `imul/idiv` past both calls; writing `r->bottom - r->top` inline at the
  call and computing `pos` from `h` moved the division back in front of
  them. **80/100 -> 59/100.** Then dropping the `range` local and writing
  `(hi - lo)` inline at both uses fixed the prologue too: **59 -> 43.**
- **A dead trailing parameter is real.** Both `Slider_Track` (5 params, 4
  read) and `RunListPicker` (9 params, 6 read) have to be *defined* with the
  full count or `/W3` warns and the caller's `add esp` is wrong.
- **A parameter's home slot is reused for a local once the parameter has
  been copied to a register.** `RunListPicker` spills `n` into the `r`
  argument slot (`E+0x10`) and homes the `rows` array in `keep_scroll`'s
  (`E+0x24`); reproducing the parameter list exactly is what makes those
  slots line up.
- **Two same-size `char` buffers are NOT ordered by declaration order**
  (confirming DECOMP): swapping `dir` and `fname` in `BrowseForFile`'s
  declaration list left the frame byte-identical. (Escalation: their order
  followed the missing scalar slot — see "Closing the two large WIPs".)

## Original bugs reproduced

- `JcDeco_Add` (and `JcDeco_Remove` before it) dereferences the
  bounds-checked cell fetch without a null check: an off-map decoration
  faults on `or word ptr [0 + 0xc], di`.
- `PickLLIDBElement` and `BrowseForFile` both allocate and free an icon
  array that the picker never reads (its `icons` parameter is dead), so the
  folder/file/drive sprites are loaded, filled in and killed without ever
  being drawn.
