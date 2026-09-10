# Scope PORT-M5 — the port lanes' recovery findings, and the last three asm rasterisers

> **Status: READY (2026-09-12).** Branch `scope/PORT-M5` from `07f11d66` (the
> PORT-A5 merge). 37 `LEGOLAND/*.c` touched, every one VC6-gated: `audit.py`
> PASS with **425 `[OK]` rows all byte-exact (mismatch=0)** and 14 `[WIP]` rows
> at the figures their own markers record, `relocs.py` **0 MISMATCH** per file
> AND tree-wide (`--all`: 254 files, 3281 functions, 0 mismatches), 3281 exact /
> 42 WIP unchanged. Both toolchains build; `ctest` **9/9 native, 15/15 wasm32**,
> including the new `coaster_span` (40 checks). **Census: asm stubs 6 -> 3**, and
> the three left are the ST(0)-ABI helpers PORT-B5 proved unreachable — every
> other inline-asm body in the game now has a C fallback.
>
> Five name disagreements settled (section 1), PORT-M3's three open callback
> slots closed (section 2) — one of which was a WRONG CALLEE NAME the byte gates
> could not see — PORT-B5's transposed texel formula fixed in both headers and
> its `sub_458930` finding audited end to end (section 3: 111 sites, 95 changed,
> and a table expectation that the shipped binary contradicts), and the last
> three asm span fillers ported with a test (section 4).

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


## 2. PORT-M3's three open callback slots

### 2a. `ObjDef +0xa0` / `DrawBasicPath` — an ORIGINAL defect, unreachable

PORT-M3 suspected "a second registration bug of the PORT-M2 1a kind", i.e. the
wrong CALLEE stored in the slot. It is not. The original stores the same
address the C does:

```
  0x00452c3d: mov dword ptr [esi+0x98], 0x45dbe0    ; AddBasicPath
  0x00452c47: mov dword ptr [esi+0x9c], 0x45dc90    ; RemoveBasicPath
  0x00452c51: mov dword ptr [esi+0xa0], 0x45dcf0    ; <- DrawBasicPath
```

What IS wrong is the pairing. `+0xa0`'s contract is fixed by its only consumer
in the whole tree:

```c
/* renderview.c:304 and :1221 */
SpriteDesc*  (*draw)(void* ctx, BPos base);   /* +0xa0 */
...
if (def->flags & 0x400) { if (def->draw == 0) goto next;
                          desc = def->draw(def->ctx, base); ... }
```

and the nine other bodies registered in it are all
`<desc>* <Class>_GetDrawDesc(elem, base)` (ridecb8.c, ridecb9.c, screencb*.c).
pathtile2.c's 0x0045dcf0 is `void DrawBasicPath(int tile, int x, int y, int
mode)` — a four-argument PAINTER in a two-argument GETTER slot — and a `.text`
scan finds no other reference to its address anywhere in the image, so there is
no second consumer that would fit it.

**Why the shipped game survives it.** `+0xa0` is read only behind
`flags & 0x400`, and the PATH CONTROL class never raises bit 10:

* bit 10 is set by a class's OWN `+0xa4` create handler — `def->flags |= 0x420`
  in mechrides.c (x6), catapult.c, joust.c, ridecb8.c, bigscreens.c — and
  `SetCustomCallbacks`'s PATH CONTROL arm registers no create handler at all;
* and it is not in the data either. Over all **155 `.odf` members of the three
  shipped RES volumes**, the class-flags dword at ODF+0x1c (the loader reads the
  first block straight onto `ObjDef+0x04`, so `flags` at +0x1c is file offset
  0x1c) uses only bits 0-2 and 16-25. **Bit 10 is never loaded from disk.**

So the mis-registration is an original defect that cannot fire. Were it ever
reached, the original would call `DrawBasicPath(ctx, base, <garbage>,
<garbage>)` and index `g_tile_sprites` with a pointer. The portable arm
registers `ll_cb_a0_DrawBasicPath`, which has the SLOT's type (so the slot holds
one wasm type and nothing else in the table has to move) and traps rather than
forwarding two invented arguments into a wild read. `test_callback_types.c`'s
note for the slot is updated.

### 2b. `schoolcar3.c hooks[slot]` — the second parameter is a FLOAT, and slot is never 3

Two facts, and neither is the one PORT-M3 guessed at ("the recovery of one of
the three [collectors] is wrong about its parameters").

**The parameter.** `o->hooks` is `DrawObj+0x4c`, which is `g_car_class_vt`'s
eight-entry group for the curve's kind (schoolcar.c's `CarClassTablesInit`,
0x00422210), and the bodies in entries 0..5 — `TrackCurve_CubicOffsetPlus`,
`CubicPosition`, `CubicOffsetMinus`, `CubicTangent` and the Line and Arc
families — are all `void f(const Curve* curve, float t, Vec3f* out)`
(schoolcar8.c, coastertiny.c). The dword this file passes as `elem` is a
FLOAT'S BIT PATTERN, the segment's curve parameter. The asm pushes it straight
out of the list:

```
  0x00428cf0: mov eax, [ebp+8]            ; the list
  0x00428cf6: mov ecx, [eax]              ; one DWORD from it
  0x00428d04: push esi                    ; ... pushed as argument 2
  0x00428d06: call dword ptr [edx+ecx*8+4]   ; hooks[slot].get_dir, stride 8
```

This is the PORT-M2 section 3 class and the same fact coaster10.c's
`TrackCurve_EvaluatePosition` / `_EvaluateUp` already carry; the portable arm
casts to the bodies' real type and moves the bits with `LL_ASFLT`. The body is
a WIP and its argument spelling is a codegen lever, so the matching arm is
untouched.

**The slot.** Slot 3 of each group is (entry 6, entry 7) =
(`TrackCurve_GatherParams` / `GetLimits` / `GetQuarterTurnSamples`,
`TrackCurve_NormalAt` / `LineUpVector` / `CubicUpVector`), and entry 6 is the
COLLECTOR at `vt+0x18` that schoolcar.c's `DrawTrackEnd_Fetch` has already
called to produce this routine's `list` and `n`:

```
  0x00428e7d: push 0x612178               ; the float array
  0x00428e82: push esi                    ; the object
  0x00428e86: call dword ptr [eax+0x18]   ; int f(obj, float* out) -- TWO args
  0x00428e96: push eax                    ; ... its return IS `n`
  0x00428e9f: call 0x428cb0               ; Coaster3D_BuildTrackMesh
```

So the three collectors are correctly recovered as `int f(X*, float* out)`;
they are simply not position hooks, `slot` at this site is a RAIL index
(offset-plus, centre, offset-minus) and nothing indexes slot 3 through the pair.
A bonus correction falls out: `void** list` is really `float*`, and
`extern int g_612178` is really that float array.

### 2c. `coaster8.c` attach/detach — and a WRONG CALLEE NAME behind the second

PORT-M3 listed both as "return-type-only" mismatches. One is. The other was a
name pointing at the wrong body, and it had a real consequence in the portable
build.

**`detach` (+0x18) is `RouteSeat_ReleaseCar` (0x004273f0), not
`RouteSeat_DetachCar` (0x004273e0).** The registration is explicit:

```
  0x004274d3: mov dword ptr [eax+0x10], 0x4273c0   ; IsOccupied
  0x004274da: mov dword ptr [eax+0x14], 0x4273d0   ; AttachCar
  0x004274e1: mov dword ptr [eax+0x18], 0x4273f0   ; <- RELEASE, not DETACH
  0x004274e8: mov dword ptr [eax+0x1c], 0x427410   ; Update
```

and the two routines are different:

```
  0x004273e0: mov ecx,[esp+4] / mov eax,[ecx+0xc] / mov [ecx+0xc],0 / ret
              -- coastertiny.c's RouteSeat_DetachCar: clears the seat, returns
                 the car
  0x004273f0: mov eax,[esp+4] / mov eax,[eax+0xc] / test eax,eax / je +
              push eax / call dword ptr [eax+0x24] / pop ecx / ret
              -- coaster9.c's RouteSeat_ReleaseCar: invokes the CAR's release
                 method and LEAVES seat->car set
```

coaster8.c's extern carried the right ADDRESS comment (`/* 0x004273f0 */`) with
the wrong NAME, so `relocs.py` and `audit.py` were both satisfied — the byte
stored is 0x004273f0 either way. The portable build links by NAME, so it
installed `RouteSeat_DetachCar` in the slot and silently stopped releasing cars
(`RouteNode_ClearSeats`, unref1.c, is the only caller of the slot). This is the
exact failure mode PORT-M4 warned about, in the other direction: the byte
evidence proves the address, never the name. Renamed for both builds — same
immediate, so nothing moves — and the rename also DISSOLVES the declared type
mismatch, because `RouteSeat_ReleaseCar` is already `void (RouteSeat*)`.

**`attach` (+0x14) is a genuine return-type-only mismatch.** 0x004273d0 is
`CoasterCar* RouteSeat_AttachCar(RouteSeat*, CoasterCar*)` and both call sites
(`RouteNode_SeatCar`, unref1.c, and coaster8.c's own store) drop the value.
A discarded cdecl return costs the caller nothing on x86, which is why the
`void` spelling matched; a wasm32 indirect call through a `-> void` type cannot
reach an `-> i32` body. Fixed in the portable arms of the two files that declare
the struct.

**And one pure alias closed:** unref1.c had a second name for 0x004273e0,
`RouteSeat_TakeCar`, with identical types. It takes the defining file's
spelling, so `gen_link.py` has one name instead of two.

`update` (+0x1c) is left alone and documented: coaster9.c's
`RouteSeat_Update(RouteSeat*, Transform*)` takes a second argument the struct
does not declare, but nothing in the tree calls the slot, so there is no
indirect call site to type.

## 3. PORT-B5's two findings

### 3a. The transposed texel formula — documentation only, in BOTH headers

The correct address is `texels[(high16(v) << ushift) + high16(u)]`: **V is the
ROW**, scaled by the shift at descriptor +0x00. PORT-B5 corrected tri3d.c's
`LLTexDesc` comment and left the two file-header paragraphs. Both are now fixed,
with three witnesses recorded at each:

* the asm: `movzx edx, word ptr [ebp-0x26]` in `DrawFlatTexTri` is the V
  accumulator's high word and it is what `shl edx, cl` shifts;
* **texture.c's C is checked against the disassembly, and the C is right.**
  `BuildTextureRecord` (0x004437d0) is an exact body, and its inner loop is
  row-major with +0x00 set from `img->w`:

  ```
    0x00443929: movsx eax, word ptr [ebp+8]    ; img->w
    0x00443933: imul  eax, edx                 ; ... * y
    0x0044393d: mov   esi, dword ptr [ebx+8]   ; tex->texels
    0x00443940: add   esi, eax
    0x00443949: mov   byte ptr [esi+edi], cl   ; ... + x
  ```

  i.e. `texels[img->w * y + x]`, which is exactly what the C says. So the ROW is
  what gets multiplied by the width, and the prose was the only thing wrong;
* unref7.c's separate hand-written `SampleTexturePixel` (0x00488730) computes
  `(cv << shift) + cu`.

`test_coaster_span` now drives the same fact a third way, through
TrackShade_FillPoly (section 4).

### 3b. The `sub_458930` audit — 111 sites, 95 of them changed

**The helper rounds, and this is not a guess.**

```
  0x00458930: fistp dword ptr [0x667c3c]
  0x00458936: mov   eax, dword ptr [0x667c3c]
  0x0045893b: ret
```

Three instructions, no `fnstcw`/`fldcw`, so it takes whatever RC the control
word holds. The only routine in the image that writes the control word is
coastershade2.c's sibling `Raster_SetFloatMode` (0x004236f0), and it CLEARS RC:

```
  0x00423713: and ax, 0xfcff      ; PC  = 00 (single)
  0x00423717: or  ax, 0x3f        ; all exceptions masked
  0x0042371b: and ax, 0xf3ff      ; RC  = 00 (round to NEAREST-EVEN)
  0x00423723: fldcw word ptr [ebp-2]
```

and it skips the reload when the word is already in that state
(`test ch,0ch / je`). The CRT's start-up word is 0x027f, also RC=00. So RC is
nearest everywhere, and every `(int)f` the original routes through 0x00458930
ROUNDS while C truncates.

**The census.** `port-m5-ftol.py` (scratchpad) disassembles every
`// FUNCTION:`/`// WIP-FUNCTION:` body with capstone and records each
`call`/`jmp` whose target is 0x00458930: **111 sites (110 calls and one tail
`jmp`) in 43 functions across 34 files.**

| file | function | sites | verdict |
| --- | --- | --- | --- |
| tri3d.c | BuildChannelTables | 2 | ROUND |
| tri3d.c | MakeShadedColour | 6 | ROUND |
| tri3d.c | ShadeLookup | 1 | ROUND |
| tri3d.c | BuildRecipTable | 1 | ROUND — a real divergence, below |
| schoolcar6.c | Shade_BuildRamp | 6 | ROUND |
| ridemisc2.c | JungleCruise_BuildWobbleTable | 6 | ROUND |
| coaster11.c | Span_ClipPlane | 2 | ROUND |
| coaster12.c | TrackPiece_FindIndex | 2 | ROUND (both joints are `float h`) |
| coaster4.c | DrawSupportShadow | 2 | ROUND |
| schoolcar3.c | Coaster3D_BuildTrackMesh | 1 | ROUND (+ a prose fix) |
| schoolcar4.c | SchoolCarBlockedAhead | 2 | ROUND |
| schoolcar.c | SchoolCarAccelerate | 1 | ROUND |
| renderview.c | RenderFullMap | 9 | ROUND (4 source sites) |
| popup.c | DrawPopUpInfo | 1 | ROUND (one call shared by two arms) |
| fpui.c | RenderFreePlayBar | 1 | ROUND |
| mantex.c | RiderTrackToScreen | 2 | ROUND |
| pathbuild.c | BNVPath_GetBINVScreenCoords | 2 | ROUND |
| bnvmove.c | SetBlokePositionFromBNV | 1 | ROUND |
| bnvpath.c | UpdateBlokeFromBNVPath | 3 | ROUND |
| bnvpath.c | BNVPath_SetDFrame | 1 | ROUND |
| logflume6.c | LFBoat_Draw / LFBoat_Fall | 1 / 2 | ROUND |
| logflume7.c | LFPath_Point | 3 | 2 ROUND, 1 insensitive (`floor`) |
| mappath.c | BuildWalkPath | 2 | ROUND |
| simcore.c | ScanBlokeSurroundings | 1 | ROUND |
| goldrush2.c | Fort_StepVisitor | 2 | ROUND |
| goldrush3.c | MoveToPanEdge / KneelAtPan / StandUpFromPan | 1 each | ROUND |
| bigsim.c | Garderner_Repair / Mechanics_Repair | 1 each | ROUND |
| workers2.c | IterateNoneWorkersRepairOrders | 1 | ROUND |
| movie2.c | MovieTicks / PrimeMovieAudio / UpdateMovieAudio | 1 each | ROUND |
| ridemachine2.c | Copters_UpdateCarRider | 1 | ROUND |
| unref1.c | LFPiece_ShadeForRow | 1 | ROUND |
| unref2.c | Raster_DrawLine | 2 | ROUND |
| bnvmove.c | CalcMoveLine | 3 | 1 ROUND, 2 insensitive (`floor`) |
| math3d.c | ArcTan256 | 1 (tail `jmp`) | insensitive (`floor(a + 0.5)`) |
| schoolcar5.c | Route_StepFree | 1 | insensitive (`ceil`) |
| bswater3.c | BsBoat_Animate | 15 | 4 ROUND (the arcs), 11 insensitive |
| roads.c | JcBoat_Animate | 15 | 4 ROUND (the arcs), 11 insensitive |

**16 sites are INSENSITIVE** and are documented in place rather than changed:
a `floor()` or `ceil()` result is already integral, so rounding and truncation
agree; and the two boat animators' straight and drift arms convert
`(integer) * 16.0f + integer`, which has no fraction either. The remaining
**95** are behind `#ifndef LEGOLAND_PORTABLE` arms whose `#else` uses
`LL_FISTP` (float) or `LL_FISTPD` (double).

**THE ONE THAT WAS ALREADY WRONG IN A TEST.** tri3d.c's `BuildRecipTable` is

```
  0x0048655e: fdivr qword ptr [0x4ab558]    ; 65536.0 / (float)n
  0x00486564: call  0x458930                ; ... ROUNDED
  0x0048656d: mov   dword ptr [esi+0x798004], eax
```

so the shipped `g_recip[30]` is **2185** (65536/30 = 2184.533) where a C cast
gives 2184, and `g_recip[99]` is **662** where a C cast gives 661 — in the 16.16
reciprocal table all four triangle rasterisers divide with, and the reason no
triangle may span more than 99. PORT-B5's `tri_raster` asserted
`g_recip[n] == 65536 / n`, the C integer division, at exactly those two n; the
expectation is corrected with a pointer to the disassembly. No n in 1..99 lands
on an exact .5 (that would need `n | 131072` with n not a power of two), so
there is no tie to break and round-half-to-even never arises.

**One DECOMP prose finding, recorded not byte-changed.** schoolcar3.c's
`Coaster3D_BuildTrackMesh` header said the vertex light is "truncated by the
game's __ftol helper". It is ROUNDED. Corrected in place; the matching body is
unaffected, because the C cast is exactly what VC6 lowers to the call.

## 4. The last three asm bodies

`TrackShade_FillPoly` (coaster13.c 0x00428860), `Span_FillShade`
(coastershade2.c 0x0041fd80) and `Span_FillShadeZ` (0x0041ff80) now have C in
their `#else` arms. The `__asm` arms are untouched and every body's audit row is
unchanged, so the matching build cannot have moved.

What each one's arithmetic is, and where it is easy to get wrong:

* **the span** is `[x0, x1]` INCLUSIVE. All three bias BOTH row pointers to x1
  and count a NEGATIVE index up to zero (`xchg ebx,eax / sub ebx,eax /
  lea edi,[edi+eax*2]`, then `add ebx,1 / jle`), which is a
  `do {} while (n <= 0)` and not a `for`;
* **Span_FillShade's 48-bit shade.** `ror ecx,16` splits the 16.16 value: ecx
  keeps the integer half in its low word (`and ecx,0ffffh`, taken UNSIGNED) and
  becomes a POINTER by adding `g_shade_clamp_mid`, while eax keeps the fraction
  in its HIGH half (`and eax,0ffff0000h`). Per pixel `add eax,g_span_dshade_lo`
  sets CF and `adc ecx,g_span_dshade_hi` walks the pointer by the integer step
  plus that carry. The table lookup is then free: `mov dl,[ecx]`;
* **the `sbb` arm is deliberate.** A negative `grad[1]` is negated and `flip`
  raised, and the negative loop uses `sbb` for the integer half while still
  ADDING the fraction. That is rate-correct: carries out of `frac + |step|`
  occur exactly as often as borrows out of `frac - |step|`. The test pins it by
  driving `dshade = -0.5` and requiring the mirrored sequence;
* **Span_FillShadeZ packs both into one register.** eax is seeded
  `(z >> 8) & 0xffffff` and `g_span_dshade_lo` packs
  `(grad[1] & 0xffffff00) << 16` (the shade fraction's top eight bits, into bits
  24..31) with `(grad[2] >> 8) & 0xffffff` (the Z step, into bits 0..23), so one
  `add` advances both; `and eax,0feffffffh` AFTER the carry clears bit 24 so a Z
  carry cannot pollute the shade. Its integer shade is taken SIGNED
  (`sar ecx,16`, no mask) — unlike Span_FillShade's — so a negative shade
  indexes BELOW `g_shade_clamp_mid`, which is what "mid" is for;
* **the Z key** is the LOW WORD of `z >> 16` and the test is a 16-bit UNSIGNED
  `>=` (`cmp cx,[esi+ebx*2] / jb skip`), so an EQUAL key overwrites;
* **TrackShade_FillPoly's texel address** is
  `((g_span_mask & V) | U) >> 16` masked with `g_span_tmask`, where
  `g_span_mask = (int)0xff000000 >> shift0`: V contributes the ROW bits and U
  the column bits — the same addressing as tri3d.c, section 3a. Both shifts of
  the combined word are LOGICAL (`shl edx,8 / shr edx,cl` and `shr ecx,16`)
  while the U scale is ARITHMETIC (`sar eax,cl`). The asm reaches the texture
  and the colour LUT through esi with two precomputed stack biases
  (`esi - tex` and `(esi - lut) >> 1`), which is only a way to free a register;
  the effective addresses are `tex[index]` and `((short*)lut)[texel]`.

### The test: `coaster_span`, 40 checks, 0 failed, both toolchains

`portable/tests/test_coaster_span.c`. The oracle is **not** a second span
filler (PORT-B3/B5's rule). Two things make the checks readable: the ramp is
built so `ramp[k] == 0x8000 | k` and the clamp table so
`clamp_mid[k] == (unsigned char)k`, so a painted pixel reports the index that
reached it; and `g_span_lut` is built from `g_shade_tab[i][grad[0]]` so a
TrackShade pixel reports its TEXEL value.

* **the plumbing is validated first** through `Span_FillFlat`, which was ported
  earlier: if the key/edge list or the row seeding were wrong, every judgement
  about the other three would be worthless;
* the span is `[x0, x1]` inclusive and NOTHING outside it or outside the
  scanline range is touched (one whole-surface check per filler);
* `dshade = 1.0` gives `0,1,2,...` and `dshade = 0.5` gives `0,0,1,1,2,2,3,3` —
  the second is a direct read of the carry chain and not of any one instruction;
* `flip` with `-0.5` gives `40,40,39,39,38,38`: the mirrored rate, which is the
  `sbb`-with-an-`add`-fraction claim;
* a span narrower than 0x8000 paints nothing and still steps the per-scanline
  shade;
* Span_FillShadeZ: the Z key written is `(z >> 16)` stepped by `dz`; smaller
  loses, EQUAL overwrites, larger wins, each as its own draw; a negative shade
  indexes below `g_shade_clamp_mid`; and a Z carry out of bit 23 leaves the
  shade index alone (the `and eax,0feffffffh`);
* TrackShade_FillPoly: `g_span_lut[i] == g_shade_tab[i][grad[0]]`;
  `g_span_tmask == w*h - 1` out of the texture header; **V is the ROW** —
  a texture whose value is its row, with v constant while u sweeps, gives ONE
  value for the whole span, and the other way round gives one row per pixel;
  U is the fast axis; `u = 16` wraps to column 0 through `g_span_tmask`; and the
  same three-way Z test.

The one structural liberty taken: the original splits the span loop on `flip`
BEFORE entering it (two loops) and the C tests `flip` inside one loop. Same
arithmetic, and the compiler hoists it.

## 5. Gate results

Every `LEGOLAND/*.c` this branch touches, at `audit.py` and `relocs.py`:

| file | `[OK]` | `[WIP]` | audit | relocs MISMATCH | section |
| --- | --- | --- | --- | --- | --- |
| advisor.c | 10 | 0 | PASS | 0 | 1e |
| bigsim.c | 4 | 0 | PASS | 0 | 3b |
| bnvmove.c | 8 | 0 | PASS | 0 | 3b |
| bnvpath.c | 6 | 0 | PASS | 0 | 3b |
| bswater3.c | 8 | 0 | PASS | 0 | 3b |
| coaster11.c | 17 | 2 | PASS | 0 | 3b |
| coaster12.c | 22 | 2 | PASS | 0 | 3b |
| coaster13.c | 16 | 1 | PASS | 0 | 4 |
| coaster4.c | 7 | 0 | PASS | 0 | 3b |
| coaster8.c | 12 | 0 | PASS | 0 | 2c |
| coastershade2.c | 8 | 0 | PASS | 0 | 4 |
| fpui.c | 18 | 1 | PASS | 0 | 3b |
| goldrush2.c | 2 | 0 | PASS | 0 | 3b |
| goldrush3.c | 6 | 0 | PASS | 0 | 3b |
| logflume6.c | 7 | 0 | PASS | 0 | 3b |
| logflume7.c | 3 | 0 | PASS | 0 | 3b |
| mantex.c | 5 | 0 | PASS | 0 | 3b |
| mappath.c | 4 | 0 | PASS | 0 | 3b |
| movie2.c | 8 | 0 | PASS | 0 | 3b |
| pathbuild.c | 6 | 0 | PASS | 0 | 3b |
| popup.c | 2 | 1 | PASS | 0 | 3b |
| renderview.c | 0 | 2 | PASS | 0 | 3b |
| ridemachine2.c | 5 | 0 | PASS | 0 | 3b |
| ridemisc2.c | 5 | 0 | PASS | 0 | 3b |
| roads.c | 2 | 0 | PASS | 0 | 3b |
| schoolcar.c | 49 | 0 | PASS | 0 | 3b |
| schoolcar3.c | 3 | 1 | PASS | 0 | 2b, 3b |
| schoolcar4.c | 7 | 0 | PASS | 0 | 3b |
| schoolcar6.c | 5 | 0 | PASS | 0 | 3b |
| screen.c | 3 | 0 | PASS | 0 | 2a |
| screens3.c | 73 | 0 | PASS | 0 | 1a-1d |
| simcore.c | 5 | 1 | PASS | 0 | 3b |
| texture.c | 6 | 0 | PASS | 0 | 3a |
| tri3d.c | 21 | 0 | PASS | 0 | 3a, 3b |
| unref1.c | 30 | 1 | PASS | 0 | 2c, 3b |
| unref2.c | 12 | 1 | PASS | 0 | 3b |
| workers2.c | 9 | 1 | PASS | 0 | 3b |
| **37 files** | **425** | **14** | **PASS, 0 REJECT / FAIL / COMPILE FAILED** | **0** | |

**Every one of the 425 `[OK]` rows is byte-exact** — `ours == orig` and
`mismatch=0`, with no exceptions — and **all 14 `[WIP]` rows are at the
instruction, byte and mismatch figures their own markers record**
(`TrackShade_FillPoly` 254i/765B against 254i/771B, 147;
`Coaster3D_BuildTrackMesh` 151i/442B against 151i/441B, 6; `Span_ClipPlane`
179i/604B against 179i/593B, 171; `RenderFullMap` 1161i/4212B, 864, ESCAPES —
every one as written in its marker before this lane). Nothing moved.

**Tree-wide relocations** (`relocs.py --all`, the HANDOFF sweep): 254 files,
3281 functions, **0 mismatches**, 26625 matched of 28256 relocations, 1631
unresolved (string and float literals, jump tables — the usual set). Worth
running here because three of this lane's commits rename callees, and a rename
to a same-sized wrong target is exactly what the byte gate cannot see.

**`progress.py --check`: 3281 exact / 42 WIP, 665/675 exports exact (98.5%),
unchanged.** `docs/LEGOLANDPROGRESS.HTML` is regenerated and committed for the
reason PORT-M1 predicted: the report records a line number per function and an
inserted `#ifndef` moves them.

**A second, cheaper proof that VC6 cannot have seen a change**
(`port-m5-vc6view.py`, scratchpad): each touched file is reduced to what the
preprocessor leaves with `LEGOLAND_PORTABLE` undefined — `#ifndef` arms kept,
`#ifdef` arms and the `#else` bodies of `#ifndef` dropped — comments and
whitespace stripped, and compared with the same reduction at `07f11d66`.
**33 of the 37 files: IDENTICAL.** The four that differ are exactly the four
identifier renames (advisor.c, coaster8.c, screens3.c, unref1.c), every one of
whose bodies is `[OK]` at `mismatch=0`.

### Builds, from the committed tree

```
native   cmake -S portable -B portable/build -G Ninja -DPython3_EXECUTABLE=$PY
         ninja -C portable/build
         ninja -C portable/build legoland_tests legoland_cbtypes
         ctest   ->   9/9 passed
wasm32   emcmake cmake -S portable -B portable/build-wasm -G Ninja \
             -DCMAKE_BUILD_TYPE=Release -DLL_ILP32=ON -DPython3_EXECUTABLE=$PY
         ninja -C portable/build-wasm
         ninja -C portable/build-wasm legoland_headless legoland_tests \
             legoland_pathtest legoland_browser legoland_cbtypes
         ctest   -> 15/15 passed
```

`legoland_cbtypes` (PORT-M3's compile-time slot/body type check) still compiles
with `-Werror=incompatible-function-pointer-types`, which is what proves the
three callback fixes of section 2 did not break another slot.

## 6. The census

`portable/tools/linkreport.py portable/build-wasm` — 258 objects, 6588 defined
symbols, 48 undefined:

| category | count | note |
| --- | --- | --- |
| **asm stubs** | **3** | was 6 at `07f11d66` |
| alias (stale extern names) | 0 | |
| game-fn / game-data / host / duplicates | 0 | |
| crt | 44 | libc/libm at link time |
| prototype conflicts | 410 | 411 at PORT-M3's close |
| unclassified | 4 | all `portable/`-side (`_stack_pointer`, `_indirect_function_table`, `_errno_location`, `_small_sprintf`) |

**The three asm stubs left are the ST(0)-ABI helpers PORT-B5 proved
unreachable** — `FastSqrt` (0x00426ab0) and `FastRSqrt` (0x00426980), whose only
references are the two `lea` stores inside their own init routines (both inside
`#ifndef` arms, with `ll_FastSqrt` / `ll_FastRSqrt` installed instead), and
`sub_458930` itself, for which no portable caller can exist because clang lowers
a float-to-int cast to an instruction rather than to a call. **Every other
inline-asm body in the game now has a C fallback.**

## 7. What this lane did NOT close

* **`ScriptEndIconInput` (0x00474fa0)** — our name for the BUTTON that calls
  `ShowStepHint`. Nothing in the binary names it, and "show the step hint again"
  reads more like a help button than a stop button. Flagged in its comment.
* **`ObjDef +0x8c`, `+0xb0` and the `interfaces.c` library triple** — PORT-M3's
  "wait for the consumer" rows (29, 18 and 5 bodies). No call site exists in the
  image, so there is nothing to type them against; they are still traps.
* **`g_report_setters[25]` (0x004b7e38) and `g_lt_action_handlers`
  (0x004b8368)** — interior words of larger blocks, so PORT-M3's table census
  cannot name their entries. Untouched.
* **The 16 insensitive `0x00458930` sites** are documented in place, not
  changed. If a later lane wants uniformity, section 3b's table says exactly
  which they are and why each is safe.
* **`prototype conflicts` is still 410.** It is the generator's own census of
  declaration disagreements, most of them `void` against `int` returns that
  PORT-M1 already made harmless; it is not a list of traps.
