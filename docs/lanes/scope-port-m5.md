# Scope PORT-M5 — the port lanes' recovery findings, and the last three asm rasterisers

> **Status: IN PROGRESS (claimed 2026-09-12).** Branch `scope/PORT-M5` from
> `07f11d66` (the PORT-A5 merge).

What this lane owes:

1. the four caller/definition **name disagreements** PORT-M4 recorded, plus
   `0x00443dc0` (`SetVidAnim` vs `StartAdvisorClip`) — decided from the
   disassembly and the callers, renamed for both builds, evidence recorded;
2. PORT-M3's **open callback slots** — `DrawBasicPath` in `ObjDef +0xa0`,
   `schoolcar3.c hooks[3]`, `coaster8.c attach`/`detach`;
3. PORT-B5's findings — the transposed texel formula in the tri3d.c /
   texture.c HEADER prose, and the **`sub_458930` audit** (the `(int)<float>`
   helper that rounds to nearest);
4. the last three inline-asm bodies ported in their `#else` arms —
   `TrackShade_FillPoly` (coaster13.c), `Span_FillShade` and `Span_FillShadeZ`
   (coastershade2.c) — with tests.

## 1. The five name disagreements — all five settled against the CALLER

PORT-M4 proved the ADDRESS at each of these call sites and stopped there,
because the byte evidence never proves a name. What settles a name is the
BODY at that address (it either does what the caller's prose says or it does
not) plus, in one case, a string the shipped binary carries.

In four of the five the DEFINING file was right and `screens3.c`'s declaration
was wrong, which is the expected direction: a declaration's prose is a guess
made from the call site, the definition's name is a guess made from the
instructions. The fifth (0x00443dc0) goes the other way, and it is the only
one of the five where the answer comes from the binary rather than from
reading code — `screens3.c`'s `SetVidAnim` was never a guess at all.

| address | screens3.c said | the defined name | verdict | what decided it |
| --- | --- | --- | --- | --- |
| 0x0046b700 | `EndScript`, "ends the running script" | `ShowStepHint` (eventmake.c) | **ShowStepHint** | the body puts a hint up; nothing unlinks or frees |
| 0x0048a800 | `SelectProfileSlot`, "re-reads the selected profile into CurProfile" | `ResetFreePlayTable` (frontend2.c) | **ResetFreePlayTable** | the body is a table walk over `g_fp_table`; no profile is read |
| 0x0048f9f0 | `PlayTitleMovie`, "starts the intro movie" | `SaveFrontEndState` (frontend2.c) | **SaveFrontEndState** | the body is nine `mov`s from globals into three out-params, and 0x0048fa40 is its exact mirror |
| 0x00474750 | `ClearObjectMenuIcons`, "drops the side panel's object icons" | `CloseActiveThemeButton` (popupmisc.c) | **CloseActiveThemeButton** | the body raises a theme's closed flag and swaps its button sprite; the caller clears the flag for the theme it then opens |
| 0x00443dc0 | `SetVidAnim` | `StartAdvisorClip` (advisor.c) | **SetVidAnim** | a shipped debug breadcrumb, and the breadcrumb convention is proved by a name we already had |

### 1a. 0x0046b700 is `ShowStepHint`, not `EndScript`

```
  0x0046b700: mov  ecx, [0x66879c]        ; g_script_cur
  0x0046b708: je   0x46b74e               ; -> xor eax,eax / ret        (no step: 0)
  0x0046b70a: mov  eax, [0x668614]        ; g_last_hint
  0x0046b711: je   0x46b738
  0x0046b713: mov  eax, [eax*4+0x7fe120]  ; g_hint_strings[g_last_hint]
  0x0046b71b: push 0x4b8bbc               ; "%s"
  0x0046b720: call 0x468bb0               ; AddHelpMessage
  0x0046b728: mov  dword ptr [0x668618], 1 ; g_hint_up
  0x0046b732: mov  eax, 1 / ret
  0x0046b738: push 1 / push ecx / call 0x46b6b0   ; ShowScriptStepText(step, 1)
  0x0046b743: call 0x468d00               ; ResetScriptTimer
  0x0046b748: mov  eax, 1 / ret
```

Every instruction is a display action, and the routine **returns a value**
(1 when a step was running, 0 when not) — an "end the script" routine would
have no use for one. There is no call to `FreeScriptSteps`, `UnlinkScriptStep`
or `EndScriptStep` (0x004787d0, which IS the teardown and lives in levelkw.c
under its own name). A second independent witness is already in the tree:
`uimisc3.c`'s header says "0x0046b700 (start the current step) raises [the
hint-is-up flag] when it puts a hint string up", and `docs/DECOMP.md` names
0x0046b700 `ShowStepHint` in two places.

`screens3.c` had `extern void EndScript(void);` against eventmake.c's
`int ShowStepHint(void)`, so the rename creates the return-type disagreement
PORT-M1 catalogued. Handled the established way: this translation unit keeps
its `void` (HANDOFF section 3 — an extern's type is a codegen lever) and the
`#else` arm carries the definition's `int`.

What is NOT settled is the name of the CALLER, `ScriptEndIconInput`
(0x00474fa0) — that is ours too, and a button whose action is "show the step
hint again" is more likely a help button than a stop button. Left alone and
flagged in its comment; nothing in the binary names it.

### 1b. 0x0048a800 is `ResetFreePlayTable`, not `SelectProfileSlot`

```
  0x0048a800: mov  edi, [0x4bdebc]        ; g_fp_table[0].name  (table base 0x4bdeb8 + 4)
  0x0048a80c: repne scasb                 ; strlen(t->name)
  0x0048a811: je   0x48a831               ; empty name terminates
  0x0048a813: mov  edx, 0x4bdebc
  0x0048a818: mov  dword ptr [edx+8], 0   ; <- 0x4bdebc+8 = entry+0x0c
  0x0048a81f: mov  edi, [edx+0x10]        ; next row's name (16-byte rows)
  0x0048a822: add  edx, 0x10
  0x0048a82f: jne  0x48a818
```

The loop's only store is `entry+0x0c = 0`, over 16-byte rows of the free-play
object table at 0x004bdeb8, terminated by an empty name — exactly
`for (t = g_fp_table; strlen(t->name); t++) t->f0c = 0;`. `CurProfile` /
`g_profile_unlocked` (0x0080ffe6) is never touched, and the routine takes no
arguments, so there is nothing for "the selected profile" to come from. The
caller `ProfileSlotInput` stores `g_profile_slot = p->slot` and THEN calls
this, which is presumably what produced the guess; the reset is a side effect
of changing slot, not the slot read itself.

### 1c. 0x0048f9f0 is `SaveFrontEndState`, not `PlayTitleMovie`

Direction is the whole question, and the disassembly is unambiguous — every
`mov` goes global -> out-param:

```
  0x0048f9f0: mov eax,[esp+4]    ; ui
  0x0048f9f4: mov ecx,[0x668e38] ; g_icons2_mode
  0x0048f9fe: mov [eax], ecx
  0x0048fa00: mov eax,[0x8119b0] / mov [edx], eax        ; edx = screen block
  ... three dwords from 0x8119b0..b8, three from 0x80ff80..88
  0x0048fa35: ret
```

and 0x0048fa40, nine instructions later, is the same nine moves with source
and destination swapped — `uimisc3.c` already calls that one
`RestoreFrontEndState` and already spells the three blocks
`g_movie_state_a/b/c`. A routine that "starts the intro movie" would call
something; this one calls nothing. screens3.c's three `g_movie_7cb*`
placeholders are renamed to uimisc3.c's spelling at the same time (identifier
only), so the push and the pop now read as a pair.

### 1d. 0x00474750 is `CloseActiveThemeButton`, not `ClearObjectMenuIcons`

Four identical blocks, each guarded by one dword of the array at 0x004bb094:

```
  0x00474750: mov eax,[0x4bb094]      ; g_theme_closed[0]
  0x00474757: jne 0x474779            ; already closed -> try theme 1
  0x00474759: mov eax,[0x7fdd40]      ; g_theme_legoland_off
  0x0047475e: mov ecx,[0x668eb0]      ; g_active_theme_icon
  0x00474766: mov dword ptr [0x4bb094], 1
  0x00474770: call 0x46d680           ; SetIconSprite
  0x00474778: ret
```

One icon, one sprite, one flag — no icon LIST is walked and no sprite is
killed (`KillSprite` is what the object-menu teardown in the same file calls).
The caller proves the flag's polarity, which is what makes "close" the right
verb:

```c
/* screens3.c LegolandThemeInput */
CloseActiveThemeButton();            /* put whatever was open back to OFF */
g_active_theme_icon = p;
SetIconSprite(p, g_theme_legoland_on);
g_theme_closed[0] = 0;               /* ... and this theme is now OPEN */
```

so `g_theme_closed[n] == 0` means open, the routine closes the first open
theme, and its own name was already right.

### 1e. 0x00443dc0 is `SetVidAnim` — the shipped binary says so

`RenderAdvisorIcon` (0x00443e30) stamps a debug breadcrumb into 0x00667c40
immediately before each operation it performs:

```
  0x00443e93: mov dword ptr [0x667c40], 0x4b7dc4   ; "SetVidAnim"
  0x00443e9d: call 0x443dc0
  0x00443eb6: mov dword ptr [0x667c40], 0x4b7db4   ; "AVI GetFrame"
  0x00443ec5: call 0x49e418                        ; AVIStreamGetFrame
  0x00443ecc: mov dword ptr [0x667c40], 0x4b7da8   ; "BltAdvisor"
  0x00443ee6: call 0x4659a0                        ; BltAdvisor
```

The strings are read straight out of `.rdata`: 0x4b7da8 `"BltAdvisor"`,
0x4b7db4 `"AVI GetFrame"`, 0x4b7dc4 `"SetVidAnim"`. **The decisive one is the
third.** It would be possible to argue that a breadcrumb names the caller's
own phase rather than the callee, except that 0x004659a0 was recovered as
`BltAdvisor` independently (scope PORT-B5 ported its body) and the breadcrumb
before it is exactly `"BltAdvisor"`; likewise 0x0049e418 is the real
`AVIStreamGetFrame` import. Two of the three breadcrumbs name the function
called on the next line, so the first one does too. `StartAdvisorClip` was our
invention; advisor.c now defines `SetVidAnim`, which is the spelling screens3.c
had all along. `docs/SCOPE_AC_advisor_initman.md`'s table line is updated with
a pointer back here.

## 6. Gate results

(filled in after the quiet window)
