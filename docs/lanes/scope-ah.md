# Scope AH — pop-up / help / free-play icon helpers (inventory group 15)

Branch `scope/AH` from main @ `8d9331e4`. Object prefix `/tmp/sah_`.
Worktree `.worktrees/scope-ah`. File `LEGOLAND/popupmisc.c`.

## Status

**12 of 12 exact.** `audit.py` PASS (twelve `[OK]`), `relocs.py` 0 MISMATCH /
0 UNRESOLVED (216 relocations, every literal declared by address), `/W3` clean.

## Per-function results

| Address | Name | Insns | Match | Audit | Marker |
| --- | --- | ---: | ---: | --- | --- |
| `0x0046f100` | `RenderIconsSkipGroup` | 85 | 100% | [OK] | FUNCTION |
| `0x0046f890` | `LoadIconBarGFX` | 37 | 100% | [OK] | FUNCTION |
| `0x0046f920` | `UnloadIconBarGFX` | 36 | 100% | [OK] | FUNCTION |
| `0x00470b00` | `UnloadPopUpTools` | 53 | 100% | [OK] | FUNCTION |
| `0x00471170` | `KillPopUpInfoSprites` | 212 | 100% | [OK] | FUNCTION |
| `0x00471c10` | `AddNewObjectMarker` | 39 | 100% | [OK] | FUNCTION |
| `0x00473640` | `ShowCursorErrorMessage` | 6 | 100% | [OK] | FUNCTION |
| `0x00474750` | `CloseActiveThemeButton` | 44 | 100% | [OK] | FUNCTION |
| `0x00474ed0` | `UnLoadInGameIcons` | 22 | 100% | [OK] | FUNCTION |
| `0x00475f40` | `SelectNextBuildObject` | 52 | 100% | [OK] | FUNCTION |
| `0x00476030` | `SetButtonFlash` | 8 | 100% | [OK] | FUNCTION |
| `0x00476050` | `ClearButtonFlash` | 11 | 100% | [OK] | FUNCTION |

## Names

Kept where the tree already had one: `LoadIconBarGFX` / `UnloadIconBarGFX`
(gamemain.c), `KillPopUpInfoSprites` (saveprof.c's tail-jump target),
`AddNewObjectMarker` (movie3.c), `UnLoadInGameIcons` (gameframe.c /
screens2.c), `SetButtonFlash` (eventgoalprim.c).

Named from the body:

- `RenderIconsSkipGroup(unsigned short skip)` — fpui.c's `RenderIcons` minus
  the icons of one group and minus the focus box. gameframe.c declares it
  `sub_46f100(int group)` and calls it with `0x2c3` (the pop-up group) right
  before `RenderIcons2(0x2c3, 0, 0)` paints that group on top.
- `UnloadPopUpTools` — the inverse of popup2.c's `InitPopUpTools`: kills
  PU_OK, PU_OKON, CB_CloseON, CB_Close and the three CB_BG slices.
- `ShowCursorErrorMessage(int error)` — `ShowMessage(g_cursor_error_msg[error])`
  through the 12-entry int table at `0x004ba9ac` ({0, 9, 3, 6, 9, 0xd, 0xe, 8,
  0xa, 0xb, 4, 7}). gameframe.c has it as `sub_473640(g_edit_cursor.error)`.
- `CloseActiveThemeButton` — the first theme whose "closed" flag is clear gets
  the flag set and `g_active_theme_icon` swapped to that theme's OFF sprite.
  screens3.c declares this address `ClearObjectMenuIcons` ("drop the object
  icons"); the body does not touch any icon list, so it is renamed here and
  the alias is recorded. Flag order legoland / castle / western / adventurers
  (screens3.c's note on the two disagreeing index orders holds).
- `SelectNextBuildObject` — gameframe.c's `sub_475f40`, called when
  `WorkOrderBuildObject` succeeds on the placed class. Decides what the cursor
  carries on placing (below).
- `ClearButtonFlash` — `SetButtonFlash(i, 0)` for the nine flash slots.
  movie3.c declares it `ClearIconHelp`; the slots are fpui4.c's `g_btnflash`,
  so the definition follows the sibling's name. Alias recorded.

Globals named for the first time: `g_icon_scroll_tick2` (`0x006688cc`, this
renderer's own scroll clock; `RenderIcons` uses `0x006688c8`),
`g_iconbar_loaded` (`0x006688d0`), `g_cursor_error_msg[]` (`0x004ba9ac`),
`g_build_followups[29]` (`0x004bb0a4 .. 0x004bb18c`, `{name, next}` string
pairs), and the four `.lls` / two format literals. Callee named for the first
time: `RemoveIconGroupRange` (`0x0046d590` — removes groups `[g, g+7)` from
both icon lists; only its shape was read, not matched).

## Mechanics

- **RenderIconsSkipGroup**: `dt = GetTicks() - g_icon_scroll_tick2`, capped at
  990, `UpdateSidePanelScroll(dt*5/33)`, restamp the clock, then
  `StoreClipping` / `RenderFullScreenIcon(0)`; every icon not hidden (`0x400`)
  and not in `skip` is drawn through its `render` callback (flag 8) or
  `PrintSprite`d; the focussed icon's animated image (type 2/3) is
  `LLSPlayOnce`d whether or not it was drawn; `RenderFullScreenIcon(0)` again,
  `RenderIconsExtra`, `RestoreClipping`. No `RenderThickBox`.
- **LoadIconBarGFX / UnloadIconBarGFX**: guarded by `g_iconbar_loaded`;
  GBarFrame.lls, IF_Side_BUp.lls, IF_Side_BDown.lls, IF_Sidebar1.lls into
  `0x00668828..34`, each behind its own null test; mode 4.
- **KillPopUpInfoSprites**: `if (g_popup_loaded)`: clear the flag, kill the
  nine `g_pu_bg` slices in slot order 0,1,2,**4,3**,5,6,7,8, then repairok,
  norepair, delete_on, delete, close_on, close, next, next_on, prev, prev_on,
  gardener_on, gardener, mech_on, mech, sad, norm, happy, hungry, peckish,
  full, then `UnloadPopUpTools()`. The PopUpUI sprites are read as their own
  globals here (`g_pu_spr_*`), not through bighelp.c's aggregate.
- **AddNewObjectMarker(ObjDef* cls)**: refuse at 20; `sprintf(buf,
  "NewObjIcons\\%s.bmp", cls->elem->name)`; `LoadSprite(buf, 0)` into
  `g_newobj.spr[count]`; on success record the class and bump the count, else
  `DBPrintf("Failied to open New Obj graphic %s\n", buf)` (sic). Uses
  fpui5.c's `NewObjStrip` aggregate (misc3.c names the same three objects
  `g_mock_defs` / `g_mock_sprites` / `g_mock_count`).
- **UnLoadInGameIcons**: `g_interface_loaded = 0`; `RemoveIconGroup(0x93)`;
  `g_script_end_icon = 0`; `RemoveIconGroup(0x9a)`; `g_brief_icon = 0`;
  `RemoveIconGroupRange(0xd2)`; `UnLoad_PopUpInfo()`; panel state
  `{2, 1, 0, 0x86}` (stowed, rebuild off-panel, x = -122).
- **SelectNextBuildObject**: `def = g_edit_object`, `elem = def->elem`. A class
  flagged `0x2000000` (ObjDef +0x1c) keeps itself. Otherwise the 29-row table
  is walked TO ITS END — no break, so the LAST matching row wins — comparing
  `NameCompare(row.name, elem->name)`; a hit takes `ElemID(row.next)` or, when
  `next` is 0, the element itself. Rows: CASTLE OBJ and the SQUARE_TRACK*
  family → SQUARE_TRACK; BOATING SCHOOL → its WATER; DRIVING SCHOOL → ROADS;
  the JUNGLE CRUISE pieces → its WATER; the LOG FLUME pieces → LOG FLUME
  TRACK; SHARK CAFE → BROLLY; the self rows (…WATER, …ROADS, …PUMPS, TRACK,
  BROLLY, WATER WORKS SHOWER / WATER BLOCK) map to themselves. If the chosen
  element has `type_flags & 2` it becomes the edit object
  (`SetEditObject(elem->data)`); otherwise the cursor is dropped:
  `g_edit_changed = 0`, `g_release_swallow = 1`, `g_input.flags &= ~0x1400`.
- **CloseActiveThemeButton**: see Names.
- **SetButtonFlash / ClearButtonFlash**: bounds-checked (`0 <= which < 9`)
  store into `g_btnflash`, and the nine-fold clear.

## Levers

- **RenderIconsSkipGroup**: `memset(&ctx.owner, 0, 8)` under
  `#pragma intrinsic(memset)` (fpui.c's `RenderIcons` shape) is what puts the
  zero in eax (`xor eax,eax` + two register stores) — two `= 0` stores were
  immediates, and that alone cost the whole register assignment downstream
  (ecx→eax for the sign-fix, edi→ebx for `skip`): 66/84 → 85/85 from that one
  change. `skip` is `unsigned short` (`mov di,[esp+0x18]` / `cmp word ptr
  [esi+0x14], di`). The loop guard `for (; p; p = p->next)` with `p` assigned
  after the two `GetTicks` calls sinks `push edi` past the empty-list test
  exactly as the original has it. The focussed-icon `LLSPlayOnce` sits
  OUTSIDE the `!hidden && group != skip` block (it is the join target of both
  skips).
- **SelectNextBuildObject**: the table cursor must be the biased pointer
  `const char** e = &g_build_followups[0].next`, `e[-1]` / `e[0]`, `e += 2`,
  bound `< &g_build_followups[29].next` (LP08: the read-only table is walked
  by two fields; plain `e->name` / `e->next` anchored at +0 for `[esi]` /
  `[esi+4]`, 49/52). The fail arm's store order is source order:
  `g_edit_changed = 0; g_release_swallow = 1; g_input.flags &= ~0x1400;`
  — flags first put its store before the other two (51/52). The load of
  `g_input.flags` still hoists above `pop edi` on its own.
- **ShowCursorErrorMessage**: must be `int` and `return ShowMessage(...)`;
  a `void` wrapper emits `pop ecx` for the 4-byte cleanup where the original
  has `add esp,4` (5/6 → 6/6). Same rule as the other one-call int wrappers.
- **AddNewObjectMarker**: writing `g_newobj.spr[g_newobj.count] = LoadSprite(..)`
  and then testing `g_newobj.spr[g_newobj.count]` reproduces the reload of
  both `count` and the slot after the store (a store to any global kills the
  CSE) — first build exact.
- **CloseActiveThemeButton**: an `if / else if` chain over four distinct
  sprite globals; each arm is one call so each gets its own `ret`
  (tail-duplication threshold: one call is copied). First build exact.
- **UnLoadInGameIcons**: the three `= 0` stores interleaved between the
  `RemoveIconGroup` calls in source order sink between each `push` and its
  `call` (the original's `push 0x93 / mov [flag],0 / call`). First build exact.
- **KillPopUpInfoSprites / UnloadPopUpTools / UnloadIconBarGFX**: the
  bubblecache.c shape — `if (s) { KillSprite(s); s = 0; }` unrolled with
  `xor esi,esi` as the shared zero. First build exact.

## Extern-type divergences

- `RenderIconsSkipGroup(unsigned short)` here; gameframe.c declares
  `sub_46f100(int group)`. Same dword push.
- `ShowCursorErrorMessage` returns `int` here (codegen lever above);
  gameframe.c declares `sub_473640(int)` as `void`.
- `UpdateSidePanelScroll(int step)` here (caller pushes a dword); fpui3.c's
  definition takes `char`.
- `RemoveIconGroup(int)` here, as bigscreens.c / fpui4.c; appraisal.c has
  `unsigned short`.
- `NameCompare` for `0x004aab90` as fpui5.c / interfaces.c; other files call
  it `_stricmp`.
- Aliases at one address (names are not levers): `0x00474750`
  `CloseActiveThemeButton` here vs screens3.c's `ClearObjectMenuIcons`;
  `0x00476050` `ClearButtonFlash` here vs movie3.c's `ClearIconHelp`;
  `0x007fded4` `g_newobj` (fpui5.c) vs misc3.c's `g_mock_*`; the PopUpUI
  sprites as `g_pu_spr_*` globals vs bighelp.c's `g_popup.spr_*` fields.

## Original bugs

None reproduced beyond the misspelt debug string. The follow-up walk's
"last row wins" is faithfully a full walk without `break`; no row name is
duplicated in the table, so it has no visible effect.
