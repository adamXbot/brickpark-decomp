# Scope LL12 — unreferenced (dead) functions, 0x00434810..0x0043f4f0

Branch `scope/LL12` from `main` `7d756410`. New file **`LEGOLAND/unref4.c`**
(the only `LEGOLAND/*.c` file touched) plus this note and the brief
`docs/SCOPE_LL12_unref_jungle_planeride_loaders.md`.

**Result: 9 of 11 exact (`audit.py [OK]`), 1,081 of 1,408 instructions.**
The file ends `PASS`, `relocs.py` reports zero `MISMATCH`, `/W3` is clean.

## Status

| address | name | insns | % | audit | marker as committed |
| --- | --- | ---: | ---: | --- | --- |
| 0x00434810 | `JcDeco_Create` | 4 | 100 | `[OK]` | `// FUNCTION:` |
| 0x00434820 | `JcDeco_SelectForPlacement` | 14 | 100 | `[OK]` | `// FUNCTION:` |
| 0x00434860 | `JcDeco_Add` | 119 | 100 | `[OK]` | `// FUNCTION:` |
| 0x004349b0 | `JcDeco_CalcCursor` | 94 | 92.6 | REJECT | `// WIP-FUNCTION:` |
| 0x00434b20 | `JcDeco_DrawSelection` | 7 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043e930 | `Slider_Track` | 101 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043ea30 | `RunListPicker` | 395 | 98.5 | REJECT | `// WIP-FUNCTION:` |
| 0x0043eee0 | `PickLLIDBElement` | 173 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043f0b0 | `BrowseForFile` | 313 | 60.7 | REJECT | `// WIP-FUNCTION:` |
| 0x0043f460 | `TextEntryFieldStep` | 57 | 100 | `[OK]` | `// FUNCTION:` |
| 0x0043f4f0 | `RunTextEntryDialog` | 131 | 100 | `[OK]` | `// FUNCTION:` |

Residuals for the three WIPs:

- **0x004349b0 `JcDeco_CalcCursor`** — strict 7/94, first diverging index 66.
  Instruction count, byte length, every block, every store order and every
  frame slot agree; the tail is **one scratch register out of phase** (the
  scratch cursor's `rect.top` temp lands in `ecx` where the original uses
  `eax`, and the following `origin` pair rotates `edx,eax` against the
  original's `ecx,edx`). Ruled out: all 24 orderings of the four
  `g_jc_deco_rect2` stores (the best, committed, order fixed indices 26–40);
  `--`/`-= 1`/`= x - 1` spellings of the four rect adjustments; `.flags`
  before/after `.next = 0`; `origin` as a `Pos` struct copy vs two field
  stores and either field first; `(found & 8) != 0`; `int`/`unsigned`/`char`
  for `found`; `int` returns declared for `DefaultCursor`, `ValidateCursor`,
  `ResetCursorFootprint`, `SetCursorError`; `void*` for `ProbeRiver`'s out
  parameter.
- **0x0043ea30 `RunListPicker`** — 392/398 aligned. The frame loop's abort
  value (-1) has to live in `esi` across an iteration and be
  **re-materialised at the loop header** (because `esi` is reused as the row
  cursor in the draw loop); we get the value into `esi`, but VC6 still
  ROTATES the loop, so the header's `mov esi,[sel]` + `ProcessSystemEvents`
  test are duplicated at the latch where the original has a bare `jmp` back.
  Ruled out: `while (cond)`, `do {} while (1)`, an explicit `goto` loop,
  `for (ret = -1; cond; ret = -1)`, a comma-operator condition, `ret` as a
  literal `-1` vs a copy of `sel`, `sel = -1` vs `sel = ret` at the two
  no-hit sites, `tp++` in the for-increment.
- **0x0043f0b0 `BrowseForFile`** — 190/313 aligned; the instruction COUNT is
  exact and every call, block and epilogue lines up. Two frame defects:
  (1) our frame is `0x64c`, the original's `0x650` — the original spills one
  more 4-byte scalar than we do, so **every** `[esp+x]` is 4 out; (2) `dir`
  and `fname` (both `char[0x100]`) are swapped in the frame — the original
  puts `dir` at the top of the frame (`E-0x100`) and `fname` below `cwd`.
  Ruled out for (1): `drive[4]`, `drive[8]`, `_finddata_t::name[264]`,
  `pattern[0x108]` (each fixes the frame SIZE but then shifts every scalar
  by 4 instead); `swapped`, `j` and a `node` pointer hoisted to function
  scope; storing `result` through memory. Ruled out for (2): swapping the
  declaration order of `dir` and `fname` (no effect — declaration order is
  irrelevant, as DECOMP already says).

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
  as a leading `break` inside is not.** `RunTextEntryDialog`:
  `for (;;) { if (!ProcessSystemEvents()) break; ...; if (done) break; }`
  emitted a second copy of the `ProcessSystemEvents` call at the latch;
  `done = 0; while (!done) { if (!ProcessSystemEvents()) break; ...; }`
  is exact (131/131), and the `done = 0` initialiser is free. The same
  rewrite is what `RunListPicker` still needs and does not get, because
  there the header also has to re-materialise a constant.
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
  declaration list left the frame byte-identical.

## Original bugs reproduced

- `JcDeco_Add` (and `JcDeco_Remove` before it) dereferences the
  bounds-checked cell fetch without a null check: an off-map decoration
  faults on `or word ptr [0 + 0xc], di`.
- `PickLLIDBElement` and `BrowseForFile` both allocate and free an icon
  array that the picker never reads (its `icons` parameter is dead), so the
  folder/file/drive sprites are loaded, filled in and killed without ever
  being drawn.
