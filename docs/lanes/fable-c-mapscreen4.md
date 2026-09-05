# Lane `fable-c` / `mapscreen4.c` — map screens and input

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
