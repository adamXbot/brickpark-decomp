# Scope PORT-M21 — P5-1's wrong address, and the hole in the gate that hid it

> **PORT-M21 — Status: DONE (2026-09-12)** — branch `scope/PORT-M21`, cut from
> `origin/main` `d8d4d368` (the PORT-P5 merge). Brief: `docs/SCOPE_PORT_WAVE.md`.
> Replay: `portable/src/browser/replays/m21-01-the-worker-drop-lands-on-plain-ground.js`.

**Result in one line: P5-1 is closed in the C — `CheckWorkerOnMouseStatus` now
reads the cursor Y at `0x00813a48`, which is what the shipped binary reads, so a
carried worker can be put down on plain ground in both builds; and
`tools/relocs.py` now set-compares the addresses a WIP body names, which
reproduces P5-1 from the source alone and, run tree-wide, finds 16 differences
across 9 other WIP bodies — none of them a second P5-1, all four of their classes
explained below. The byte gate did not move and could not: the fix changes one
displacement inside one relocation, so `audit.py`'s output for `workers2.c` is
byte-identical before and after (WIP row 184i/672B, mismatch 82, 55.4%).**

---

## 1. The defect, and why it is a matching fix rather than a portable arm

`workers2.c` frames the input block at `0x00813a40` for itself, and the frame
had a field that does not exist:

```c
typedef struct CursorState {
    int           flags;        /* +0x00  0x813a40 */
    Pos           point;        /* +0x04  0x813a44  (Pos is two ints) */
    int           y2;           /* +0x0c  0x813a4c   <-- a phantom */
    unsigned char buttons;      /* +0x10  0x813a50 */
```

and the drop's vertical range test read it:

```c
if (g_cursor.y2 < 0x20 || g_cursor.y2 >= 0x174)   /* 0x00813a4c */
    goto fail;
```

The original reads `0x00813a48` — `point.y`, the cursor's screen Y — and holds
it in `eax` across both compares:

```
0x004706dd: mov  eax, dword ptr [0x813a48]
0x004706e2: cmp  eax, 0x20
0x004706e5: jl   0x470895              ; fail
0x004706eb: cmp  eax, 0x174
0x004706f0: jge  0x470895              ; fail
...
0x00470701: mov  eax, dword ptr [0x813a44]   ; point.x -- which we had right
```

`0x00813a4c` is bighelp.c's `g_input.mouse_a.MASK`, the Controller bit the left
button is bound to: written once by `SetupControllers` and a constant **1** for
the whole session (measured live, every reading in this lane's park). `1 < 0x20`
is true, so the body took `goto fail` on every pixel, `g_drag_lock` stayed 1,
and the tail's `SetWorkersPositionAtMouse()` re-parked the worker under the
cursor on every tick — for ever. Neither `0x00813a4c` nor any other word of the
`mouse_a` pair appears anywhere in the original's 672 bytes.

So this is a transcription error shared by the VC6 build, not a wasm artefact,
and the fix is in the game C with no `#ifdef` of any kind:

```c
if (g_cursor.point.y < 0x20 || g_cursor.point.y >= 0x174)
    goto fail;
```

The struct field is **kept**, renamed `mouse_a_mask`, with a note saying what it
really is — it holds the layout out to `buttons` at `+0x10`, and the next reader
must not be able to mistake it for a second Y.

### Why no gate saw it, and why no gate moved

`g_cursor` is one global either way, so the instruction is the same five bytes
with a different displacement, and the normalised comparison every byte gate
uses blanks exactly that displacement. `audit.py LEGOLAND/workers2.c` is
**byte-identical** before and after the fix (nine `[OK]` rows unchanged, WIP row
`184i/672B mismatch=82`), `/W3` is clean, and the marker set is unchanged.
`relocs.py` is the one gate that resolves the real addresses — and it skipped
this body, because its comparison is positional and a WIP body's instructions do
not line up with the original's. That is §2.

---

## 2. The gate gap: `WIPRELOC`

A WIP body's order cannot be compared, but the SET of addresses it names can.
`tools/relocs.py` now does that for every `// WIP-FUNCTION:` marker:

* the **original's** side is recovered from the operands, because a linked body
  has no relocation table: every 32-bit displacement or immediate whose value
  lands inside the image (`0x00401000`..`0x00836000`), plus every direct
  call/branch whose target leaves the body. A branch that stays inside is a
  label, not a reference.
* **our** side is read from the object's own relocations and resolved with the
  same annotations the positional check uses. A label in our body, anything that
  resolves inside the original's extent (a recursive call), and an unresolved
  literal are claimed as references by neither side.
* the differences print as `WIPRELOC <file> <fn> <va> MISSING original=0x...` /
  `... EXTRA ours=0x...`, **never** as `MISMATCH`, and the WIP counts are in
  neither exit-status bit. `relocs.py --all | grep MISMATCH` is therefore exactly
  the gate it was, a clean sweep still exits 2, and no round can start failing
  because of a WIP body it never touched. `--no-wip` turns the pass off.

Proved both ways on `workers2.c`, same build, same command:

| tree | output |
| --- | --- |
| P5-1's read restored | `WIPRELOC ... MISSING original=0x00813a48 (the original reads it at 0x004706dd: mov eax, dword ptr [0x813a48] [data])` and `WIPRELOC ... EXTRA ours=0x00813a4c symbol=_g_cursor`, 0 MISMATCH |
| fixed | nothing; the body's **19 references are 19/19 shared** |

### What the first cut got wrong, and the four things now accounted for

The first version printed **132** WIPRELOC lines tree-wide and P5-1 was two of
them; 76 were `MusicThread`'s MIDI-name strings. A lead list that size is not a
lead list, so four classes are now accounted for:

1. **a string or float literal, by CONTENT.** The linker puts our copy wherever
   it likes, so its address cannot be derived — but the bytes can be compared: if
   the original holds our exact literal at the address it names, that is the same
   literal somewhere else, not a different object.
2. **a switch table, by class and count.** VC6 emits the jump table and the byte
   case-index table into the function's own COMDAT just past the code, so the
   original's copies sit immediately after the original body (`[end, end+512)`,
   reached through an index register) and ours are local `$L` symbols.
3. **a bitwise instruction's immediate is a mask, not an address**
   (`or dword ptr [esp + 0x48], 0x800000`), and `0x00400000` is the image base,
   below the first section.
4. **a reference that resolves inside the original's own extent is our code** —
   a recursive call, which read back as `EXTRA` because the original's is a
   branch we correctly dropped as a label (`RenderCursor`, `BsRoute_Trace`).

Tree-wide that is 132 lines → **16**, 0 EXTRA, 112 accounted. `--self-test`
covers the new path on the synthetic COFF: both sets, a label-only body, a mask
immediate, a table operand, a wrong global's MISSING/EXTRA pair, the coincidence
the set test cannot see (a wrong `g_base` whose `-4` form still lands on an
address the original names), literal content matching, the recursive-call
exclusion, and that `print_result` never emits `MISMATCH` for a WIP row.

---

## 3. The tree-wide sweep — the next brief's raw material

`$PY tools/relocs.py --all` on the branch tip: **0 MISMATCH**, exit 2, 258 files,
3281 exact bodies checked, **42 WIP bodies checked, 0 skipped**, 1,627 unresolved
(the documented normal), `wip_references_shared` 821, `wip_references_accounted`
112, `wip_references_extra` **0**, `wip_references_missing` **16** across 9
bodies. Every one of the 16 is explained, and **none is a second P5-1**:

| file | WIP body | MISSING | what it is |
| --- | --- | --- | --- |
| `renderview.c` | `RenderFullMap` 0x004567a0 | 0x004ab080, 0x004ab09c, 0x004ab0c0, 0x004ab0c4, 0x004ab0d0 | **GDI32 import slots with no IAT annotation** — `SelectObject`, `DeleteObject`, `LineTo`, `CreatePen`, `MoveToEx` (resolved out of the PE import directory). The body calls them; the five `__declspec(dllimport)` declarations at renderview.c:1658-1662 carry no `/* [0x004ab0xx] */` comment, so `relocs.py` cannot claim them. **The one item here worth fixing**: annotate them and a wrong import in that body becomes visible. |
| `coaster11.c` | `Span_ClipPlane` 0x0041f050 | 0x00458930 | `__ftol`, the compiler's float→int helper. No `extern` exists to annotate (the compiler emits the call), so our side has the symbol and no address. |
| `popup.c` | `DrawPopUpInfo` 0x004724a0 | 0x00458930 | same |
| `renderview.c` | `RenderFullMap` | 0x00458930 | same |
| `schoolcar3.c` | `Coaster3D_BuildTrackMesh` 0x00428cb0 | 0x00458930 | same |
| `appraisalscreen.c` | `RunAppraisalScreen` 0x004453a0 | 0x0049e600 | `_chkstk`, the stack probe — called from the body's FIRST instruction, so it is the frame size, not a name. |
| `renderview.c` | `RenderView` 0x0045b180 | 0x0049e600 | same |
| `musicthread.c` | `MusicThread` 0x00492db0 | 0x004a0833 | another CRT helper (a word-copy routine at `0x004a0833`), same class as `__ftol`. |
| `musicthread.c` | `MusicThread` | 0x004959f8 | a **switch jump table** the accounting could not pair: the original's `MusicThread` dispatches through one more table than ours does. A real (if small) shape lead for a lane on that body. |
| `person3d.c` | `Draw3DPersonModel` 0x00440a30 | 0x00500000, 0x005a0000 | **false positives.** `mov ecx, 0x500000` / `mov ecx, 0x5a0000` are the 16.16 constants 80.0 and 90.0 — PORT-M17's `ox`/`oy` — fed to `FMUL`. They are read as addresses because the image's zero-fill tail reaches `0x00836000`. Unfixable from the operand alone; `mov reg, imm32` is how both are spelled. |
| `coaster12.c` | `Raster_AddSpanRecord` 0x00423200 | 0x004e3870 | **false positive of a second kind.** Our VC6 arm spells the original's bound as the bare constant `0x004e3870` (coaster12.c:618), and a C integer literal emits no relocation, so there is nothing on our side to compare. PORT-M9 documented the pair (`g_cmd_buf` + 0x6000) in the portable arm above it. |

So the class P5-1 belongs to — a WIP body reading a *different same-sized global*
— occurs **exactly once** in the tree, and it is now fixed. The residual list is
one annotation chore (renderview.c's five imports), one switch-shape lead
(`MusicThread`), and seven compiler-generated calls plus three literal
false positives that no operand-level rule can remove. **Nothing else in this
table was touched by this lane.**

---

## 4. The proof in the browser, and the A/B

`legoland.html?args=-nointro+WINDEBUG&awake=1`, tutorial lesson 2, on port 8915.
`M21.run()` was run twice; the only difference between the two runs is that one
word of C, rebuilt with `ninja -C portable/build-wasm legoland_browser` and the
page reloaded. Arm B therefore IS the unfixed build, not PORT-P5's `p5-04`
approximation of it from the outside (a `0x21` written into the mask).

Setup is identical in both: the hedge pen on screen, a Gardener picked up at
about (288,177) (`dragLock` 1, `carrying` the Bloke), dropped on plain ground at
cell **(46,13)** = screen **(166,246)**, hit `0x109`, cursor Y **246** (inside the
real band 0x20..0x174), and the word at `CursorState +0x0c` reading **1** in both
arms.

| probe | arm A (fixed) | arm B (the word put back) |
| --- | --- | --- |
| hover (46,13), in hand | worker (46,13), lock 1 | worker (46,13), lock 1 |
| **CLICK — the drop** | **lock 1 → 0** | **lock stays 1** |
| hover (46,13) | (46,13) | (46,13) |
| hover (48,13) | (46,13) — stays | (48,13) — follows |
| hover (44,12) | (46,15) — walking | (44,12) — follows |
| cursor → (200,100) | (46,23) | (38,3) — follows |
| cursor → (350,200) | (43,26) | (49,4) — follows |
| cursor → (500,300) | (41,22) | (59,6) — follows |

In arm A the worker is on the ground: it ignores the cursor from the second probe
on and then walks out of the hedge pen under its own AI. In arm B its own 24.8
world position at `+0x68/+0x6c` equals the hovered cell **every** time, 150-pixel
jumps across the park included — `SetWorkersPositionAtMouse` from the tail on
every tick. `llWorkers().gardeners` is 4 throughout both arms (nobody is lost or
duplicated), 0 traps in both, 35.2–35.7 fps in both, cold-load hash
`0x093021ac` unchanged.

**One trap for the next lane, and PORT-P5 recorded it first:**
`llWorkers().carrying` (`g_worker_on_mouse`, `0x007fdff0`) is **not** cleared by a
successful drop, so `P5.drop()` returns `ok:false` on a drop that worked and
`llWorkers()` alone cannot answer "is the worker placed". The oracle is
`g_drag_lock` plus the worker's own world position — and reading that position
*through the stale pointer* is what makes the table above decisive: same pointer,
same worker, and in one arm it tracks the cursor while in the other it does not.

---

## 5. The WIP body: still 55.4%, and two more shapes refuted

The brief asked for the match to be taken as far as it reasonably goes. It did
not move, and the floor recorded above the function (Scope I, Scope LL18, and the
six passes before them) stands: **184i/672B, 82 strict mismatches, first at
index 102**, which is *one missing instruction* plus one `cmp`/`test` choice —
the off-by-one shift makes the other 80 rows of the diff.

* the original's out-of-range arm at `0x4707a3` is `mov ebp,1 / mov [g_drag_lock],ebp /
  call SetWorkersPositionAtMouse / epilogue`, i.e. a *copy* of the `fail` block
  with a rematerialised const-1 in front of it; ours folds the store to
  `mov dword ptr [g_drag_lock], 1` and is one instruction short there.
* **new refutation 1.** The obvious reading of that copy — "the source says
  `goto fail` and VC6 tail-duplicated the block, prepending the remat the web
  needs because `xor ebp,ebp` killed it for the map height" — is **wrong**. With
  the arm written as `else { goto fail; }` VC6 deletes the arm instead of
  duplicating the target: 668 bytes (4 short), the whole const-1 web collapses to
  immediates, and the match falls to **27.7%** (51/184).
* **new refutation 2.** The other mismatch is `cmp eax, edi` (ours) against
  `test eax, eax` (the original) at the join after the two placement calls.
  Writing the two calls as their own tests (`if (SetGardenerWorkOrderAtPostion(...))
  goto tail;` in each arm, no `r`) in the hope that VC6 cross-jumps them into the
  original's single `test` gives 102/184 again but **678 bytes (+6) and ESCAPES**
  — it would fail the extent gate.

Both shapes are now on the record next to the nine the earlier passes tried. The
honest statement is that the residual is a scheduling artefact of that build's
threading/allocation order, it is not the address this lane came for, and the row
is **no worse than 55.4%** — it is identical to base, to the byte.

---

## 6. Gates

| gate | result |
| --- | --- |
| `audit.py LEGOLAND/workers2.c` | **byte-identical to base**: 9 `[OK]` rows unchanged, WIP `0x00470620` `184i/672B mismatch=82`, `PASS: 0 function(s) failed the extent gate` |
| `relocs.py LEGOLAND/workers2.c \| grep MISMATCH` | empty (10 UNRESOLVED, all literals/jump tables; WIP row 19/19 shared) |
| `relocs.py --all \| grep MISMATCH` | **empty**, exit 2; 3281 exact + 42 WIP checked, 0 skipped, 16 WIPRELOC MISSING (§3), 0 EXTRA |
| `/W3 /O2 /Gy /Gd` on `workers2.c` | clean |
| marker set (`grep -ho '^// FUNCTION: LEGOLAND 0x...' LEGOLAND/*.c \| sort`) | **identical**, 3281 lines |
| `progress.py --check` | green; `3281 exact / 42 WIP` (report regenerated — only workers2.c's ten line numbers move) |
| `extern_sweep.py` | 0 multi-address extern statements |
| `bvstruct_sweep.py` | 0 unaccepted silent sites (1 silent, 5 noisy — base) |
| `port_m10_bvstruct_sweep.py` | 0 slot-vs-body sites |
| `addr_sweep.py` | identical to base: 1 name at two addresses, 5 addresses of incompatible size — diff against the base `workers2.c` is the **line number alone** (`workers2.c:247` → `:257`) |
| native build + ctest | clean configure/build, **19/19** |
| wasm build + ctest | clean configure/build, **26/26** |
| browser | 0 traps, 35.2–35.7 fps, cold-load hash `0x093021ac` |

`tools/verify.py` was not run (integrator's gate).

---

## 7. Owed, and one disclosure

* **`renderview.c`'s five `dllimport` declarations have no IAT annotation** and
  that is why five of the 16 MISSING lines exist. One line each; deliberately
  NOT done here, because this lane's brief says not to touch the other bodies
  and an annotation changes what the gate claims about a body it does not own.
* **`MusicThread` dispatches through one more jump table than ours does** — the
  only shape lead in the residual list.
* **the two `person3d.c` immediates and `coaster12.c`'s bare bound will keep
  printing.** They are the documented price of reading the original's side out of
  its operands: a 32-bit constant inside the image range is indistinguishable
  from an address, and a C literal emits no relocation to compare it with. If
  they ever become noisy, the answer is an accepted-baseline file, not a looser
  rule.
* **P5-1's sibling is still open and is not this lane's:** `g_worker_on_mouse` is
  never cleared when a worker is removed (§4). PORT-P5 filed it as a loose end;
  it is a one-word game-behaviour question (the shipped binary does the same), so
  it needs a measurement before anyone "fixes" it.
* **Disclosure about the shared scratchpad.** The session scratchpad this lane
  was given is shared with earlier lanes. Before noticing that, this lane wrote
  two generically named files into its root — `env.sh` and `markers_before.txt`
  — either of which may have overwritten an earlier lane's copy (both already
  existed as names in that directory; the timestamps are now this session's).
  Everything afterwards is under `<scratchpad>/m21/` with an `m21-` prefix. If a
  later lane finds its `env.sh` rewritten, that is this lane's doing.
