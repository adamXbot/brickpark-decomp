# Scope P — the game frame: 10 of 10 exact

**Status: complete.** Branch `scope/P`, baseline `origin/main` `d6a5671e`
(2026-09-05). One new file, `LEGOLAND/gameframe.c`, 10 functions, 1,403
instructions, every one `audit.py [OK]`; `/W3` clean; `relocs.py` 547 of 547
resolved positions agree, 3 unresolved (the in-text jump tables of `GameFrame`
and `InGameFrame`, as expected). No existing file was edited.

## Per function

| address | name | insns / bytes | audit | marker | note |
| --- | --- | ---: | --- | --- | --- |
| 0x00458bc0 | `EnterFrontEnd` | 4 / 31 | OK | `// FUNCTION:` | first try |
| 0x00458be0 | `CompleteLevelForProfile` | 7 / 30 | OK | `// FUNCTION:` | first try; screens3.c declares it `sub_458be0` |
| 0x004588c0 | `ShowTitleScreen` | 31 / 110 | OK | `// FUNCTION:` | first try; the brief's `ShowWaitSprite` renamed: it blits `TitleScreen1.lls` |
| 0x004589a0 | `ResetController` | 45 / 168 | OK | `// FUNCTION:` | first try |
| 0x00458a50 | `StartPark` | 45 / 198 | OK | `// FUNCTION:` | first try; screens3.c declares it `sub_458a50` |
| 0x00458b20 | `BeginParkLoad` | 29 / 144 | OK | `// FUNCTION:` | first try; screens3.c declares it `sub_458b20` |
| 0x00458830 | `ReadExeVersionString` | 49 / 129 | OK | `// FUNCTION:` | first try |
| 0x00458c00 | `GameFrame` | 153 / 708 | OK | `// FUNCTION:` | first try (jump table in `.text` at 0x00458ec4) |
| 0x00458ee0 | `InGameFrame` | 289 / 1119 | OK | `// FUNCTION:` | 79% → 100% in four levers (below) |
| 0x00457a70 | `HandleMapClick` | 751 / 2889 | OK | `// FUNCTION:` | 75% → 100%; the identity check needed one more lever after the normalised gate passed |

Placeholders to rename at integration: `screens3.c` declares `sub_458a50`,
`sub_458b20` and `sub_458be0` for `StartPark`, `BeginParkLoad` and
`CompleteLevelForProfile`. This file declares the still-unmatched callees
with the tree's placeholder names (`sub_457870`, `sub_489ee0` from uimisc2.c
and Codex-F; `sub_48a750`, `sub_48a800`, `sub_48a040`, `sub_46cb20`,
`sub_483090`, `sub_49cfc0`, `sub_474ed0`, `sub_498b40`, `sub_4969d0`,
`sub_46f100`, `sub_46ee00`, `sub_46cff0`, `sub_482cb0`, `sub_455fc0`,
`sub_450a40`, `sub_4632b0`, `sub_44db90`, `sub_457970`, `sub_475f40`,
`sub_473640`), all with the address comment.

## Mechanics recovered

**`GameFrame` (the per-frame dispatcher).** Order every frame: `sub_498b40`;
then exactly one of three pending transitions — open the map screen
(`g_map_screen_pending` 0x008119bc: freeze the clock, `InitOptionSamples`,
clear both icon handlers, clear flag 0x20, `InitMapScreen`,
`DisableInfoPopUPIcons`, `DisableSidePanelIcons`), close it
(`g_map_screen_leave` 0x0080ff70: `ResumePausedSamples`, handlers cleared,
flag 0x20 set, `KillMapScreen`, `SetInGameIconHandlers`,
`EnableSidePanelIcons`, thaw), or start a park (`g_park_start_pending`
0x00667c64: freeze, `ResetGameClock`, `ResetSaveTimer`, handlers cleared,
`g_castle_placed = 0`, `g_ms_options_dirty = 1`, `InitOptionSamples`; then
either a saved game — `DeletePlayableSamples(0)`,
`sprintf("%s\\%dsave%d.sav", "profiles", profile_slot, save_slot & 0xff)`,
`SetWaitSpriteRect(0,0)`, `LoadGame`, `ClearWaitSprite`,
`InitGameInterface(0)`, `SetInGameIconHandlers` when `g_game_load_pending`
0x00667c80 — or `KillHelpText` + `StartPark`; then `UpdateSoundVols`,
`g_pending_state = 0`, `g_game_mode = 3`, `SetInGameIconHandlers`, thaw).
Then `switch (g_game_mode)` through the 4-entry table at 0x00458ec4: 0 →
`return 0` (the main loop ends), 1 → `MapScreenFrame` (scope Q), 2 →
`SetPointer(5); InitScreens((char)g_screen_mode)`, 3 → resume samples if
`g_ms_options_dirty`, then `InGameFrame`. Then, unless a park start is
pending: `g_dbg_where = "SFX"`, `sub_4969d0`, `ReadGameButtons`,
`g_sim_frame++`, and the level-end machine when `g_pending_state && mode == 3`:
play `g_level_end_movie` if `g_level_end_has_movie` (pointer 0 during, 5
after); state 3 → `g_icons2_mode = 0`, front end screen 1, `BeginParkLoad`;
otherwise state 1 advances `g_map->level`, then `sub_48a750`,
`BeginParkLoad`, `g_icons2_mode = 1`, front end screen 6; `UpdateHelpBar`;
`return 1`.

**`InGameFrame`.** Phases in `g_dbg_where`: `"AI"` (`HandleRideAI`,
`DoMapAI`, `ControlPeople`, `ControlWorkers`, `CheckWorkerOnMouseStatus(0)` if
a worker is on the mouse), `"ProcessStuff"` (`ProcessBuildingTimes`,
`ProcessDamage`), the hit type is read here — BEFORE `ResetHitInfo` —
`"Zoning"` (`SetPointer(5)`, `ResetHitInfo`), `"Rendering"`
(`PushRenderingStatusAndLockVideoSurface`, `RenderView`, then `HandleMapClick`
when the saved hit type has bit 0x100 or a drag is in progress), the overlay
(`PrintSprite(InterfaceBG, 0, 0, 0, &{1,0,0})`, `sub_46f100(0x2c3)`,
`sub_46ee00`), `"In Game Help"` (`ProcessInGameHelp`, `DrawPopUpInfo`,
`RenderIcons2(0x2c3,0,0)`, `sub_46cff0`), with the hit triple
{type, value, cell} saved across the overlay and restored during a drag;
then the bubble help by hit type in edit mode 0 (0x103 object: pointer 8 and
the class name; 0x10a/0x10b/0x10c/0x10d strings 0xd4/0xd2/0xd3/0x7e4 with
pointers 8/7/7/8; 0x306 visitor name via `GetVisitorName` + `sub_482cb0`;
0x307/0x308 strings 0x90/0x92 then `sub_450a40(value)`; anything else
pointer 7) or the selected class's help in edit mode 2; hit type 5 with the
button held → `KillAdvisorHelp`, `g_icon_clicked = 1`; `"Appraisals"`
(`sub_44db90`) / `"Appraisals Over"` (`UpdateFocussedIconPtr`); the focussed
icon (pointer 6, `CheckFocussedIcon`) unless dragging or clicked; the worker
sprite on the mouse; `PopRenderingStatus`; `sub_4632b0` when
`g_show_capacity` and a shift key is down; `RenderingComplete`;
`"Exiting GameProc"`.

**`HandleMapClick`.** Classification (edit mode 0 or 2, hit type 0x100 or
0x103): the cell under the cursor (`g_input.map` inside the map, non-null
row) decides — flags & 0x800: a work order (gardener first, then mechanic;
0x10b / 0x10c, or the old type when neither), the cell position into
`g_hit_info.cell`, and the selected class from the cell's object; flags &
0x88 without 0x800: the object itself (0x103), and if the object is
`ElemID("DRIVING SCHOOL ROADS")` and the road record's bit 0x10 is set the
value becomes `ElemID("ZEBRA CROSSING")`; neither: tile-info lookup
(`g_tile_info[tile].range->flags[tile - base] & 0x10` → 0x10d, else 0x109).
Off the map: keep a 0x103 hit in mode 2, else 0x10a. Then by edit mode: 0 =
query (object / work-order hits call slot +0x94 of the selected class with
the cell position and show the query cursor; anything else resets the query
cursor's rect, flags = 8, `SetCursorError(c, 1)`, origin = map ref, clears
flag 0x400, `g_667c5c = 0`, and drops the drag class unless the button is
held). 1 = place (`SetPointer(4/3)` by cursor validity, flag 0x800 by
env/hedge class, slot +0x90 of the edit class with the screen point unless
flag 0x400, `BuildCursorPtr(edit, 0x8f8, IsBuildableClass)`; on a button:
a valid cursor with a drag-placeable class fills the drag rectangle —
for the env class first clearing path cells (`ClearCellForPath`) — with
`sub_457970` + `WorkOrderBuildObject` per footprint step, marking
`g_map_dirty |= 0x10`, `g_path_gfx_batch = 1`, and playing the build effect
+ sample once; a non-drag class builds once at the cursor origin
(`sub_475f40`, `g_map_dirty |= 2`); an invalid cursor calls
`sub_473640(error)`; no button just renders the cursor; button 1 released
returns to mode 0). 2 = destroy/query (the same slot-+0x94 / cursor-reset
split as mode 0 unless dragging, in which case `UpdateMapDrag` and the edit
cursor is copied wholesale (0x1834 bytes); on a button a drag-removable
class removes every matching object in the drag rectangle
(`RemObjFromMap`), else a valid cursor erases the work order (0x10c/0x10b)
or removes the selected object — dereferencing a NULL cell when the
position is off the map, an original bug; then a 0x103 hit sets or clears the
drag class by its 0x2000000 flag). Finally a released left button with no
icon clicked, mode 0 and no worker on the mouse opens the pop-up info for
the hit record at the cursor point.

## Globals and types named or confirmed

`g_hit_info` 0x004bdd00 is ONE 12-byte record {type, obj, cell}; the tree's
`g_hit_type`/`g_icon_value`/`g_hit_cell` are its fields (this file keeps the
field names for InGameFrame and the aggregate for HandleMapClick). `g_edit`
0x008119b0 is {mode, game_mode, object} = `g_edit_mode`, `g_game_mode`,
`g_edit_object`. `g_input` 0x00813a40: +0x24 is a `Pos map` (the map ref
under the cursor), +0x2c/+0x30 the footprint step, +0x44..+0x50 the drag
rectangle, +0x84 `btn0.state`, +0x8c `btn1.state`. `g_map`'s cell counts
are the u16 pair at +0x14/+0x16. `Controller` gains the three SPI_GETMOUSE
values at +0x1c..+0x24. The query cursor's rect list (+0x1414, five dwords)
is reset by a `memset`, flags (+0x1828) = 8. `0x00667c5c` stays a
placeholder (`g_667c5c`). `g_map_screen_pending` 0x008119bc,
`g_map_screen_leave` 0x0080ff70, `g_park_start_pending` 0x00667c64 and
`g_game_load_pending` 0x00667c80 are named here for the first time (the
tree had `g_8119bc`, `g_80ff70`, `g_667c64`, `g_667c80`).

## Original bugs reproduced

- `HandleMapClick`, mode 2, remove-selected arm: `c` is NULL when the
  selected cell is off the map and `c->obj->def` is dereferenced anyway.
- `HandleMapClick`, mode 1: `g_map_loading` is cleared twice, before and
  after the build-effect block.
- `ReadExeVersionString`: `malloc` result unchecked; `strcpy` into the
  caller's buffer with no length.

## Extern-type divergences (caller-side levers)

- `InitScreens(char)` here; the definition and other callers say `int`. This
  caller loads a byte (`mov cl, byte ptr [0x80ff88]`).
- `PopUpInfoSetUp(HitInfo key, int x, int y)` — the 12-byte record by value
  (elsewhere `PopUpKey`; same layout).
- `GetVisitorName(void*)`, `sub_482cb0(void*)`, `sub_450a40(void*)` take
  `g_hit_info.obj` directly.
- `CheckWorkerOnMouseStatus(int)`, `DeletePlayableSamples(void*)`,
  `RemObjFromMap(ObjDef*, MapObj*, BPosW, Cursor*)` — the packed cell is
  passed as the 16-bit union.
- The version-resource imports are declared WITHOUT `dllimport` (thunks at
  0x0049e3a0..0x0049e3ac); `SystemParametersInfoA` WITH it (IAT 0x004ab2b4).

## Levers, with evidence

- **A sub-object `memset` keeps a small struct's zeros out of the constant
  web (RC08).** `InGameFrame`'s blit context {1,0,0}: written as three
  stores or as `= {1,0,0}` the two zeros plus the guard compare formed a
  zero web in esi, pinning `push esi/edi` to the prologue (224/282). `ctx.a
  = 1; memset(&ctx.b, 0, 8)` gives `xor eax,eax` + two stores, the guard
  stays `mov/test`, and the pushes sink past the AI phase: prologue exact.
  Six type variants (pointer-typed fields, `void*` parameter, short args)
  were inert; the memset is the only spelling that split the web.
- **Push sinking needs the sunk values in a `static __inline` helper (or an
  inner block) opened after the guard** — but only once the zero web is
  gone; with the web present the helper was inert. `InGameFrame`'s body from
  "ProcessStuff" to the switch lives in `InGameFrameBody(&ctx)`.
- **Register phase of a guarded block: test the global directly and name the
  copy inside.** `if (g_edit_mode == 0 || g_edit_mode == 2) { int edit =
  g_edit_mode; ...}` puts the value in edx and x/y in eax/ecx as the
  original; `int edit = g_edit_mode` before the test, or x/y locals, or a
  Pos copy, were 1–2 registers out of phase (283 → 293 → 299/299).
- **The switch is the inline arm; the selected-class help is the exiled
  else** — `if (edit == 0) { switch } else { ... }` (the original's `jne` to
  the exile).
- **A cached global is not a local.** `HandleMapClick`'s edi is
  `g_hit_type`'s forwarded value: `mov edi, K / mov [g_hit_type], edi` is the
  store of a constant kept for later reads, and the `mov edi,[g_hit_type]`
  reloads follow calls. A local `hit` constant-propagates (`mov [mem], K`)
  and lands in ecx. Read the global at every use (also `g_sel_def`,
  `g_input.map.x/y`, `g_edit_object` in the loops, `g_icon_value` in the
  work-order arms: each is re-read after every call in the original).
- **Identical `if/else if` arms are cross-jumped at IR level unless they
  differ; name the pointer differently in each** (`d`, `d2`, `d3`): the
  original keeps three copies of the slot-+0x94 call in mode 2 with the
  scratch rotation advancing across them, and merges only the call in mode
  0 (the pushes differ in register). One shared arm was 3 instructions short.
- **The icon tail is written twice and VC6 merges the suffixes itself**;
  `goto` into it emits `jne / jmp` instead of `je`.
- **Load the definition before the two stores**: `g_sel_def = obj->def;
  g_sel_bpos.w = sq` — VC6 then orders the stores bpos, def as the original;
  the other order stores bpos before loading def. Naming `o`/`sq`/`d`
  temporaries here shifted the whole saved-register assignment (hit → ebp,
  roads → edi): only the `(f & 0x800)` arm tolerates them.
- **Byte fields stored twice need `int` temporaries** (`int bx =
  c->sq.b.x`): direct re-reads reload after the global store (aliasing),
  `unsigned char` temporaries grew the frame by 8 bytes.
- **The one-web appeared by itself** once the `int` temporaries and the
  direct global reads were in: zero → ebx, `mov ebp,1` at the dispatch, with
  `g_path_gfx_batch = 1` re-materialised as `mov ebx,1` before the build
  loop. No spelling of the constant 1 was needed (a `one = 1` local
  propagated away).
- **`if (CursorIsValid(&c)) SetPointer(2); else SetPointer(1);`** — a
  ternary argument compiles to `neg/sbb/neg/inc`; the original branches on
  the pushes and merges the call (two sites).
- **Store order after a flag update**: `g_input.flags |= 0x400;
  g_drag_class = g_sel_def;` interleaves as load-flags, store-class, or,
  store-flags; the reverse source order stores the class first.
- **`g_667c5c = 0` before `g_input.flags &= ~0x400`** in both cursor-reset
  arms (the flags store is last).
- **A second name for the same address defeats a CSE the original does not
  have.** (a) The leading test reads `g_edit.mode` (the aggregate at
  0x008119b0) while the off-map compare reads `g_edit_mode`: with one name,
  the top load survives `g_sel_def = 0` and is re-materialised into esi
  before `cmp edi,0x103`, one instruction long, misaligning 600 positions;
  a volatile at either site adds a register load instead. (b) The loops'
  bounds checks read `g_level_map` (0x004bcbf4) while the classification
  reads `g_map`: the post-call reloads come out in temporary-creation order,
  and with one name `g_map`'s older temporary reloads before
  `g_drag_class`/`g_edit_object` (4 strict, 0 register-blind). The tree
  already carried both names for both addresses, from earlier functions.
- **`g_hit_info` as one aggregate** for the by-value pop-up call: the copy
  takes its first dword from the cached edi and loads obj/cell in field
  order; three separate globals plus a `PopUpKey` local rotate the scratch
  registers one step.
- **A `Pos` copy of the cursor point** (`pt = g_input.point`) sets the
  rotation for that record copy (the argument pushes y then x): 12 → 4.
- **A `Pos` copy for the query cursor's origin in mode 0**
  (`g_query_cursor.origin = g_input.map`): VC6 moves an 8-byte struct high
  dword first, which is the only way to get the y-before-x order the
  original has there — and it was invisible to the normalised gate: `relocs.py`
  flagged the swapped identities (i=706..710) after `audit.py` said `[OK]`.
  Mode 2 assigns x then y as two scalars and is exact that way.
- **Inert here**: volatile reads at three sites, `static const`-style
  spellings, pointer/int typing of `g_sel_def`, `g_drag_lock` as a pointer,
  `PrintSprite` short arguments, `RenderIcons2` int arguments.

## Verification

```sh
$PY tools/audit.py LEGOLAND/gameframe.c        # 10 x [OK], PASS
$PY tools/relocs.py LEGOLAND/gameframe.c       # 0 MISMATCH, 3 UNRESOLVED (jump tables)
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sp_w3.obj LEGOLAND/gameframe.c
```
