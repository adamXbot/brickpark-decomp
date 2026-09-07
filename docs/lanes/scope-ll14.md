# Scope LL14 — unreferenced (dead) code, popup / iconbar / input / fpui

Branch `scope/LL14`. File `LEGOLAND/unref6.c` (new). Object prefix `/tmp/sll14_`.
Brief: `docs/SCOPE_LL14_unref_ui_help_input.md`.

Twenty-one functions the linker kept although nothing live in the binary
references them (`tools/inventory.py` classifies them DEAD; most were found
only by the padding sweep). They are ordinary C from the same translation
units as their matched neighbours — `iconui.c`, `render2.c`, `fpui.c`,
`fpui2.c`, `popup2.c`, `popupmisc.c`, `tinystubs.c`, `sysstubs.c`,
`sweep2/3.c` — and that neighbourhood supplied every struct, global and
helper name used here.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0046d3b0 | SetHelpFaceState6 | 2 | 100 | [OK] | FUNCTION |
| 0x0046d800 | AddLabelledIcon | 23 | 100 | [OK] | FUNCTION |
| 0x0046de10 | MoveIconGroupWithClip | 23 | 100 | [OK] | FUNCTION |
| 0x0046dfd0 | RenderLabelledIcon | 37 | 100 | [OK] | FUNCTION |
| 0x0046ea10 | RenderClippedClassIcon | 43 | 100 | [OK] | FUNCTION |
| 0x0046f5e0 | AddLabelledClassIcon | 59 | 100 | [OK] | FUNCTION |
| 0x0046f860 | AddClippedClassIcon | 16 | 100 | [OK] | FUNCTION |
| 0x0046f9a0 | MakeUpChildrenList | 148 | 57.1 | [WIP] | WIP-FUNCTION |
| 0x00473560 | GetCursorErrorText | 26 | 100 | [OK] | FUNCTION |
| 0x004735c0 | ResetMessageTimers | 6 | 100 | [OK] | FUNCTION |
| 0x00473680 | OpenMessageBar | 30 | 100 | [OK] | FUNCTION |
| 0x004736e0 | CloseMessageBar | 3 | 100 | [OK] | FUNCTION |
| 0x004736f0 | DrawMessageBar | 117 | 100 | [OK] | FUNCTION |
| 0x00474090 | IsCtrlDown | 8 | 100 | [OK] | FUNCTION |
| 0x004755b0 | Unref_4755b0 | 2 | 100 | [OK] | FUNCTION |
| 0x004761e0 | Unref_4761e0 | 1 | 100 | [OK] | FUNCTION |
| 0x00476220 | Unref_476220 | 2 | 100 | [OK] | FUNCTION |
| 0x00476230 | Unref_476230 | 2 | 100 | [OK] | FUNCTION |
| 0x00476240 | Unref_476240 | 2 | 100 | [OK] | FUNCTION |
| 0x00476250 | SaveResearchList | 53 | 100 | [OK] | FUNCTION |
| 0x004762f0 | LoadResearchList | 67 | 100 | [OK] | FUNCTION |

**20 / 21 exact, 522 of 670 instructions.** `audit.py` ends PASS,
`relocs.py` zero MISMATCH and zero UNRESOLVED over the 20 exact bodies,
`/W3` clean.

`0x0046f9a0` is the only WIP: exact instruction count (148) and block layout,
mismatch 139 by `audit.py`, first diverging index 1. See the residual note
above its marker and "Residual" below.

## Mechanics recovered

### A second, LABELLED icon kind (0x0046d800 / 0x0046dfd0 / 0x0046f5e0)

The icon record's polymorphic slots gain one more reading. `AddLabelledIcon`
(0x0046d800) is `InsertIcon` plus a CAPTION: it puts the caption text at
+0x18, a caption offset pair of SHORTS at +0x20/+0x22, sets the render
callback to 0x0046dfd0 and ORs flags 0x208. `RenderLabelledIcon`
(0x0046dfd0) draws the icon's sprite with the usual kind-2 `BlitCtx` and
then `PrintCent(x + cap_dx, y + cap_dy, 0x40, text, 1)` — a 0x40-wide
word-wrapped centred box, font 1.

`AddLabelledClassIcon` (0x0046f5e0) is the class-icon front-end: it takes an
`ObjDef*`, references the class sprite itself (so the class's sprite outlives
the icon), and anchors the caption under the sprite — `cap_dx = w / 2`,
`cap_dy = h + 2`, i.e. centred horizontally and two pixels below the bottom.
It then ORs 0x1000, stores the ObjDef at +0x08, the caller's f16 at +0x16 and
applies the three `g_group_cb*` overrides exactly as `AddGBarClassIcon` does.

This is the only known site where icon +0x18 is a `char*` caption, and the
only known use of +0x20/+0x22 as a 16-bit offset pair.

### A class icon that clips itself to its widget (0x0046f860 / 0x0046ea10)

`AddClippedClassIcon` (0x0046f860) forwards its six arguments straight to
`AddGBarClassIcon` (0x0046f690) and then overwrites the render callback with
0x0046ea10. **ORIGINAL BUG (reproduced):** the returned icon is stored
through with no null check, so an allocation failure faults; every other
builder in this family guards.

`RenderClippedClassIcon` (0x0046ea10) builds the icon's bounds the way
`uimisc.c`'s `GetIconBounds` does (`right = w + x`, `bottom = h + y`) and
draws only when `IntersectRect` against the owning widget's rectangle — the
16-byte `WinRect` at `widget + 0x1c` — is non-empty AND the icon has a
sprite. The sprite test is the SECOND half of the guard, so a sprite-less
icon still pays for the intersect. This is the first recorded reader of
`widget + 0x1c` as a rectangle.

### A SECOND control bar (0x00473680 / 0x004736e0 / 0x004736f0)

`popup2.c` documents the pop-up's control bar (`InitPopUpTools` 0x00470950,
`DrawPopUpExtra` 0x00471d90). This scope recovers a second, independent
instance of the same strip that was never wired up:

```
0x00668d68  active flag        0x00668964  centre-slice count n
0x007fe010  x                  0x007fe014  y
0x00668968  caption buffer (strcpy'd, no length check)
```

- `OpenMessageBar(x, y, text, ok_fn, close_fn)` (0x00473680) `strcpy`s the
  caption into the 0x00668968 buffer (intrinsic `repne scasb` + `rep
  movsd`/`movsb` — an unbounded copy into a fixed global), stores x/y,
  builds the two gadgets through the SHARED `InitPopUpTools`, and sets the
  active flag.
- `CloseMessageBar` (0x004736e0) calls 0x00470b00 (the control bar's sprite
  unloader — `UnloadPopUpTools`, named here) and clears the flag.
- `DrawMessageBar` (0x004736f0) is a near-clone of `DrawPopUpExtra`: the
  CB_BGleft / CB_BGCentre x n / CB_BGRight strip at 0x7a + n*0x20 + 0x4e, the
  same OK / Close gadget placement (`right - 0x4b`, `right - 0x27`, `y + 3`,
  flags `&= ~0x400`), the caption through `PrintCachedText` at
  `(x + 0xc, y + 6)` in 0xff0000-on-0xffffff styles 1/5, and the same
  leave-the-strip hit test calling `ResetToolIcons` on a miss on either axis.
  Differences from `DrawPopUpExtra`: it takes its geometry from its own
  globals, prints the buffer directly instead of going through
  `Format("%s")` into a stack buffer, its centre-slice loop counts UP
  (reloading the count global each iteration) instead of down, and it does
  NOT call `DisablePopUpInputs` at the end. It reproduces the SAME original
  bug: the caption box width is `n * 20 + 0x7a` (the vertical line pitch)
  where every horizontal measurement uses `n * 0x20`.

Because both share `InitPopUpTools` and the two gadget globals
(0x007fdea8 / 0x007fe000), the message bar and the pop-up control bar could
never have been up at the same time.

### The research list's own save chunk (0x00476250 / 0x004762f0)

The R&D / research list (`g_research_list` 0x00668ed8, `fpui.c`'s 12-byte
`ObjNode`) has a complete save/load pair that nothing calls:

* Write: node count, then per node `strlen(node->obj->elem->name)`, the name
  WITHOUT its terminator, and the node's `keep` flag.
* Read: the count, then per node a 4-byte length, that many bytes into a
  512-byte STACK buffer (no bound check against the stored length — a stack
  overflow on a corrupt save), NUL-terminate at the length, `ElemID(name)`,
  take its class definition from element +0x0c, and set the class's flag word
  at ObjDef +0x1c: clear 0x04000000, set 0x08000000. The list is rebuilt by
  appending, and the final node's `next` is nulled from the loop's own
  exhausted counter register.

`ObjDef +0x1c` bits 0x04000000 / 0x08000000 are named here for the first
time as the "researched"/"in research" pair.

### Small leaves

* `SetHelpFaceState6` (0x0046d3b0): `g_help_face_state = 6`, the dead
  sibling of `tinystubs.c`'s `SetHelpFaceTalking` (state 4).
* `IsCtrlDown` (0x00474090): `key[DIK_LCONTROL] & 0x80 ? 1 :
  key[DIK_RCONTROL] >> 7`. Confirms the DirectInput key-state array base is
  **0x007fdda0** (`sysstubs.c`'s `g_left_shift` 0x007fddca = +0x2a and
  `g_right_shift` 0x007fddd6 = +0x36; ours are +0x1d and +0x9d).
* `ResetMessageTimers` (0x004735c0): clears `last` in all **17** records of
  `workorder2.c`'s message table at 0x004ba8e0 (the table's length is
  confirmed by the loop bound 0x004ba9b4). `workorder2.c` already noted this
  address as the clearer; it is now written.
* `GetCursorErrorText` (0x00473560):
  `strcpy(out, GetString(g_messages[g_cursor_error_msg[err]].topic))` —
  `g_cursor_error_msg` (0x004ba9ac, `popupmisc.c`) indexes the message table
  and the message's `topic` is a STRING id, not only a help-popup topic.
* `MoveIconGroupWithClip` (0x0046de10): `MoveIcons(0xffff, group, dx, dy)`
  then pulls the group's CLIP icon (`group + 2`) the other way through a
  16-bit pair at +0x1c/+0x1e, so its clip origin stays put.
* Four `return 1` leaves and one empty `void` (see Names).

### 0x0046f9a0 — the children-class panel (WIP)

Reconstructed behaviour (the C is committed and the layout is exact):
allocate a 0x2c-byte `ObjListPanel`, build the scroll-up / scroll-down / box
icons with `AddGBarIcons`, record the box's extent in both the panel's
"list" (+0x0c..+0x18) and "box" (+0x1c..+0x28) rectangles, install
`BuildObjectIconInput` (0x00470000) as the group's input callback, then walk
EVERY LLIDB element and add an icon for each whose flags have 0x10 and whose
class definition's parent (`ObjDef +0x58`) is the caller's element. Bit 0 of
the `flags` argument selects the layout: set steps DOWN by 0x38, clear steps
ACROSS by 0x79. Afterwards it adds the full-screen icon for `group + 6`,
shrinks the box icon to the icons actually laid out and moves the
scroll-down gadget (`group + 4`) to the new edge, and — if no icon was made
at all — removes the whole group again. Returns the icon count.

This is the "children bar" counterpart of `fpui2.c`'s `MakeUpObjectList`
(0x00475960), which builds the same panel record from the object list
instead of from an LLIDB parent.

## Names

Every name in this file is ours; none of these functions is exported.

| address | name | why |
| --- | --- | --- |
| 0x0046d3b0 | `SetHelpFaceState6` | sets `g_help_face_state` to 6; the meaning of 6 is not recoverable from the disassembly, so the name states what it does |
| 0x0046d800 | `AddLabelledIcon` | `InsertIcon` + caption |
| 0x0046de10 | `MoveIconGroupWithClip` | moves a group and compensates its clip icon |
| 0x0046dfd0 | `RenderLabelledIcon` | render callback of the above |
| 0x0046ea10 | `RenderClippedClassIcon` | class icon clipped to its widget rect |
| 0x0046f5e0 | `AddLabelledClassIcon` | labelled icon for an `ObjDef` |
| 0x0046f860 | `AddClippedClassIcon` | `AddGBarClassIcon` + the clipped renderer |
| 0x0046f9a0 | `MakeUpChildrenList` | `MakeUpObjectList`'s sibling over an LLIDB parent's children |
| 0x00473560 | `GetCursorErrorText` | copies the cursor-error message's string |
| 0x004735c0 | `ResetMessageTimers` | clears the message table's `last` ticks |
| 0x00473680 | `OpenMessageBar` | opens the second control bar |
| 0x004736e0 | `CloseMessageBar` | closes it |
| 0x004736f0 | `DrawMessageBar` | paints it |
| 0x00474090 | `IsCtrlDown` | either Control key |
| 0x00476250 | `SaveResearchList` | writes `g_research_list` |
| 0x004762f0 | `LoadResearchList` | reads it back |

Five bodies carry no recoverable behaviour and use the sanctioned fallback:
**`Unref_4755b0`, `Unref_476220`, `Unref_476230`, `Unref_476240`** are
`char f(void) { return 1; }` (`mov al,1 / ret`) and **`Unref_4761e0`** is
`void f(void) {}` (a bare `ret`). Their neighbours (`InsertObjectNode` in
`fpui5.c`; `Unload_RAndDCheckBox` / `CloseCheckBoxRAndD` /
`DisableRAndDIcons` in `sweep2.c` / `sweep3.c`) make "always-accept icon
input handler" the likely reading — an icon `input` returns `char` — but
nothing in the binary confirms it, so the addresses are recorded rather than
guessed at.

`UnloadPopUpTools` (0x00470b00) is named here for the first time: it kills
and nulls the seven control-bar sprites `InitPopUpTools` loads.

## Globals first named here

| address | name | what |
| --- | --- | --- |
| 0x00668d68 | `g_msgbar_active` | second control bar is up |
| 0x007fe010 | `g_msgbar_x` | its x |
| 0x007fe014 | `g_msgbar_y` | its y |
| 0x00668964 | `g_msgbar_cells` | its centre-slice count |
| 0x00668968 | `g_msgbar_text` | its caption buffer (`strcpy` target) |
| 0x007fddbd | `g_left_ctrl` | key state [DIK_LCONTROL] |
| 0x007fde3d | `g_right_ctrl` | key state [DIK_RCONTROL] |

`g_messages[]` (0x004ba8e0) is confirmed to have exactly 17 records, and
`ObjDef +0x1c`'s bits 0x04000000 / 0x08000000 and `ObjDef +0x58` (parent
element) are recorded above.

## Extern-type divergences

* `AddGBarClassIcon` (0x0046f690) is declared here exactly as `fpui.c` and
  `fpui2.c` declare it (`short f16`); `AddClippedClassIcon`'s own `f16`
  parameter is an `int`, because the original reads it with a full dword
  `mov` and lets the conversion happen at the push.
* `AddLabelledIcon`'s caption-offset parameters are `short` (the original
  reads them `mov dx, word ptr [esp+...]`), while its `x`, `y` and `group`
  are `int` (full dword reads). The caller relies on this: it computes
  `h + 2` in 32 bits over a 16-bit load (`mov dx,[ecx+16h] / add edx,2`),
  which only happens when the callee's parameter is 16 bits wide.
* `IntersectRect` is declared exactly as `blitmisc.c` / `gpu.c` declare it.

## Levers learned (with evidence)

* **A short local materialised before a call splits into `mov r16` +
  `movsx`.** `AddLabelledClassIcon` (0x0046f5e0) went from 40/55 to 59/59 by
  replacing `AddLabelledIcon(..., (short)(d->icon->w / 2), (short)(d->icon->h
  + 2), ...)` with two `short w, h;` locals assigned first and passed as
  `w / 2` and `h + 2`. Three things move together: the original's
  `mov ax,[ecx+14h] / movsx eax,ax` two-step (an explicit cast gives the
  single `movsx eax, word ptr`), `add edx,2` on a 16-bit-loaded register (an
  explicit `(short)` cast gives the 16-bit `add dx,2`), and — because the two
  short definitions lengthen the live ranges across the argument block — the
  fourth callee-saved push (`push edi`) that the whole body's register
  phase depends on. With edi allocated, `flags |= 0x1000` encodes as
  `or edi,1000h` (6 bytes) instead of `or ch,10h` (3), because edi has no
  8-bit subregister. **The encoding of an OR of a 12-bit constant is
  evidence about WHICH register the value is in.**
* **`ctx.kind = 1; memset(&ctx.owner, 0, sizeof(ctx.owner));` is not
  interchangeable with three assignments or with `BlitCtx ctx = { 1 }`.**
  In `DrawMessageBar` (0x004736f0) the original stores `ctx.owner.p` BEFORE
  the argument pushes and `ctx.kind` / `ctx.owner.n` after them, and shares
  one `xor eax,eax` with `PrintSprite`'s literal `0` mode argument. `= { 1 }`
  hoists all three above the function's leading guard (missing the guard's
  push sinking); three plain assignments sink all three below the pushes and
  build no zero register; the `kind` + sub-object `memset` pair reproduces
  the split exactly (110/117 -> 117/117). This extends RC08's `ctx.sub`
  measurement to the kind-1 (plain) context.
* **Adjacent 16-bit field stores are emitted in the reverse of source
  order, and the source order shows in the LOADS.** `AddLabelledIcon`
  (0x0046d800): writing `p->cap_dy` before `p->cap_dx` gives loads in that
  order and stores swapped (4 mismatches); writing `cap_dx` first is exact.
  The stores stay `+0x22` then `+0x20` in both. Diff the LOAD order, not the
  store order.
* **Two adjacent global stores likewise flip.** `OpenMessageBar`
  (0x00473680): `g_msgbar_y = y; g_msgbar_x = x;` compiles to the x store
  first; the source order that reproduces the original's `[0x7fe014]` then
  `[0x7fe010]` is `g_msgbar_x = x; g_msgbar_y = y;`.
* **`register` is inert at /O2** (four spellings of `MakeUpChildrenList`,
  identical objects) — it cannot be used to force a loop counter into a
  callee-saved register.
* **Initialising a loop counter before the function's allocation guard
  builds a zero web but COSTS the counter its register.** In
  `MakeUpChildrenList`, `i = 0;` as the first statement produces the
  original's `xor R,R` / `cmp esi,R` / two `push R` zero web, but VC6 then
  gives `i` its own frame slot (`sub esp,0xc` instead of `sub esp,8`) and
  the carrier register goes on to hold a cursor. Getting the web is not the
  same as getting the allocation.
* **The nearest exact sibling bounds what is reachable.** `fpui2.c`'s
  `MakeUpObjectList` (0x00475960) compiles the same panel prologue with its
  two cursors in ebx/edi; 0x0046f9a0 has one more live value and the
  original spills BOTH cursors into dead argument slots instead. When a
  sibling with one fewer live value gives exactly the allocation you are
  getting, the residual is a ranking problem, not a statement-order problem.

## Residual (0x0046f9a0, and what a stronger model could try)

`audit.py`: ours 148i/420B vs original 148i/411B, mismatch 139, first
diverging index 1. `matchfull.py` 84/147 = 57.1%. Instruction count, block
layout, branch targets, call sequence, struct offsets and the frame size
(`sub esp,8`, four callee-saved pushes) are all the original's. The entire
residual is one register-allocation split:

```
original  esi=panel  edi=group  ebx=flags  ebp=i
          ix -> [E+4]  (group's dead arg home)
          iy -> [E+0x18] (h's dead arg home)
          count -> [E+0x14] (flags' dead arg home)
          n -> [E-8], e -> [E-4]
          constant 0 hoisted into ebp before the malloc; it serves n = 0,
          `cmp esi,ebp`, both SetNewGroup_Callbacks zero arguments, and then
          becomes the loop counter.
ours      esi=panel  ebx=group  ebp=ix  edi=iy
          i, flags, count spilled; no zero web.
```

About thirty spellings were measured and none moves the split: loop as
`for` / `while` / `continue`-guards; `n++` before and after the call;
`d = e->def` hoisted into a local; root copies of `group` and `flags` used
throughout; the tail reading `icon` vs `w->box` and `iy` vs `w->list_y1`;
`if (!w)` vs `if (w == 0)`; `i = 0` early (48.7%); `register int i` (inert).
Best measured is 57.1%; the spread over all variants is 48.7%–57.4%.

**Idea for a stronger attempt:** the two cursors must become *cheap in
memory* while `group` and `flags` stay register-worthy. The original updates
them with `add dword ptr [esp+X],K`, i.e. VC6 never gave them a register at
all. Worth trying: a spelling in which the cursors are not loop-carried
scalars — e.g. derived each iteration from a base plus the match count
(`ix = w->list_x0 + n * 0x79`) with the observed `add` recovered by strength
reduction; or a `static __inline` helper for the per-element step whose
temporaries share the spill-home pool; or making `flags` participate in a
constant web (its two `& 1` tests currently narrow to byte operands, which
RA08 says are not qualifying uses) so it is allocated ahead of the cursors.
