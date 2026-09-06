# Scope W — the script-event constructors: 72 of 72 exact

**Status: complete.** Branch `scope/W`, baseline `origin/main` `221dbe3e`
(2026-09-06, after scope T). One new file, `LEGOLAND/eventmake.c`, 72
functions, 1,132 instructions, every one `audit.py [OK]`; `/W3` clean;
`relocs.py` 218 of 218 resolved positions agree, 0 `MISMATCH`, 1
`UNRESOLVED` (the `"%s"` literal). No existing file was edited. Objects
under `/tmp/sw_*`. 69 bodies were exact on the first compile; three needed
one change each (below).

## Per function

| address | name | insns / bytes | audit | marker | note |
| --- | --- | ---: | --- | --- | --- |
| 0x0046b590 | `InsertScriptStep` | 23 / 57 | OK | `// FUNCTION:` | first try; the brief's `FreeScriptSteps` renamed from the body (a sorted insert) |
| 0x0046b610 | `LinkGoalEvent` | 10 / 30 | OK | `// FUNCTION:` | first try |
| 0x0046b630 | `LinkStepEvent` | 11 / 29 | OK | `// FUNCTION:` | first try |
| 0x0046b650 | `SetScriptStepText` | 39 / 87 | OK | `// FUNCTION:` | 2 → 0: the brief's `AddEvent_Intro`; arguments are (text, step) |
| 0x0046b700 | `ShowStepHint` | 23 / 81 | OK | `// FUNCTION:` | 19 → 0: nest the body, trailing `return 0` (below) |
| 0x0046b790 .. 0x0046bda0 | `AddEvent_Give` .. `AddEvent_Lookat` (31 step constructors) | 10–22 each | OK | `// FUNCTION:` | first try, all 31 |
| 0x0046bdd0 .. 0x0046c510 | `AddEvent_Need` .. `AddEvent_Forever` (37 goal constructors) | 11–27 each | OK | `// FUNCTION:` | first try except `AddEvent_Needin` (4 → 0: store order, below) |

Renames and name reconciliation for the integrator:

- `0x0046b590` `FreeScriptSteps` → **`InsertScriptStep(ScriptStep*)`**: it
  walks `g_script_steps` and inserts the step sorted on `id`; nothing is
  freed. `EndScriptStep` (0x004787d0, scope R) is its caller.
- `0x0046b650` `AddEvent_Intro` → **`SetScriptStepText(const char* text,
  ScriptStep* step)`** — note the argument order, read off the frame
  (`[esp+0xc]` after one push is the step, `[esp+0x10]` after three the
  text). `LevelKw_INTRO` passes `(argv[1], g_script_cur)`.
- `0x0046b700` `ShowStepHint` kept (screens3.c declares this address as
  `EndScript`, "ends the running script" — a misreading: it puts the
  pending hint string or the current step's text up as advisor help and
  returns whether a step was in progress). Rename screens3.c's extern at
  merge.
- `0x0046bb80` → **`AddEvent_Capacity(int which, int v)`** and `0x0046bd70`
  → **`AddEvent_Flashbutton(int bits, int on)`**: levelkw3.c's (scope T,
  merged) names, kept over the brief's compound names.
- `0x0046b7c0` `AddEvent_Kind3` and `0x0046c4e0` `sub_46c4e0` →
  **`AddEvent_Kind68`**: both dead (no keyword reaches kinds 3 and 68).
- The other constructors keep the brief's `AddEvent_<Keyword>` names, which
  levelkw2.c (scope S) already declares with matching argument types.

## Mechanics recovered — the event record as the constructors fill it

`ScriptEvent` (0x44 bytes; uimisc.c/fpui3.c's layout): `next` +0x00,
`elem` +0x04, `text` +0x08, `kind` +0x0c, `flags` +0x10 (byte), three
per-kind argument dwords **f14 / f18 / f1c**, a `Pos` at +0x20, a `Rect`
at +0x28..+0x34, `mode` +0x38 (always 1 from here), `time` +0x3c, and
`root` +0x40 (a goal's root goal, `g_script_root`). `ScriptStep`: `next`,
`id`, `text`, `goals` +0x0c, `events` +0x10.

Every constructor is `e = NewScriptEvent(kind, 1)`, the stores below, then
`LinkStepEvent(e, g_script_cur)` (step events, kinds 2–32, `flags = 0`,
pushed at the head of `step->events`) or `LinkGoalEvent(e, g_script_cur)`
(goal events, kinds 33–69, `flags` = the section's flags byte passed as
the first, byte-wide argument, `root = g_script_root`, pushed at the head
of `step->goals`). The three text events (FMV 12, INTERVAL 13, MESSAGE 14)
and the two file events (BRIEFINGFILE 28, HINTSFILE 29) call
`SetScriptEventText(e, s, 1)` (heap copy, flag 0x20) and do not touch
`flags` themselves; INTERVAL also zeroes f1c.

| kind | keyword | elem | f14 | f18 | f1c | pos / area |
| ---: | --- | --- | --- | --- | --- | --- |
| 2 | GIVE | elem | popup | | | |
| 3 | (dead) | elem | | v | | |
| 4 | TAKE | elem | | | | |
| 5 | ADDBRICKS | | | | count | |
| 6 | CURRENCY | | | | amount | |
| 7 | PLACE | def | | count | | pos |
| 8–11 | CLEAR, UNGLUE, GLUE, EXTENDPARK | | | | | area |
| 15 | FEATURE | | v | | idx | |
| 16 | GARDENER / MECHANIC | | who | | count | pos |
| 17 | WORKERS | | b | | a | |
| 18 | DEGRADE | def | v | | n | |
| 19 | MAX/MIN CAPACITY/VISITORS | | which (1 max, 0 min) | | v | |
| 20, 21 | CAPACITYSCALE, CAPACITYCAP | | idx | | v | |
| 22 | ENTRANCEFEE | | | | v | |
| 23 | LOOKAT | | | | | pos |
| 24 | REPORT | | b | idx | a | |
| 25, 26, 27 | THEMEICON, ADDFLAG, BRIDGES | | icon / flag / count | | on | |
| 30 | FLASHBUTTON / FLASHBUTTOFF | | on | | bits | |
| 31, 32 | PURGE, ENDLEVEL | | | | | |
| 33 | NEED | elem | | | count | |
| 34 | NEEDAT | elem | | | | pos |
| 35 | NEEDIN | elem | | | count | area |
| 36, 37 | CONNECT, LINK | elem | | | | |
| 38 | RANGE | elem | b | | a | |
| 39 | CLEARAREA | | count | | | area |
| 40 | REMOVE | elem | | | count | |
| 41 | REMOVERANGE | elem | lo | | hi | |
| 42 | COMPOSITE | elem | extra | | count | |
| 43 | LOOPCOMPOSITE | elem | | | count | |
| 44 | TECHLEVEL | elem | | level | | |
| 45 | PARKVISITORS | | | | count | |
| 46, 47 | RIDEVISITORS, RIDERS | elem | | | count | |
| 48–53 | the six COVERAGE goals | | pct | | | |
| 55 | STUDAREA | | count | | | area |
| 56 | SAVE | | | | slot | |
| 57 | HAPPINESS | | b | | a | |
| 58, 59 | NEEDGARDENERS, NEEDMECHANICS | | | | count | |
| 60 | HUNGER | | b | plus | a | |
| 61 | FIXRIDES | | b | | a | |
| 62 | POWERRIDES | | | | count | |
| 63 | ZONING | | count | | zone | |
| 64 | CHECKFLAG | | flag | | on | |
| 65, 66, 67 | SELECTTHEME, SELECTTAB, SELECTMODE | | | | index | |
| 68 | (dead) | elem | | | | |
| 69 | FOREVER | | | | | |

Two irregularities worth knowing before the tick handlers (scopes V, X)
name these fields: the six coverage goals keep their percentage at **f14**
where every other single-value goal uses f1c, and CHECKFLAG stores `(flag,
on)` as (f14, f1c) while ZONING stores `(zone, count)` as (f1c, f14).

**`InsertScriptStep`**: walk `g_script_steps` while `step->id < s->id`,
remembering the predecessor; splice after it. With no predecessor the step
becomes the head **with a NULL link** — a step that sorts before the
current head drops the rest of the list (original bug, reproduced).

## Original bugs reproduced

- `InsertScriptStep` loses the list when inserting before the head (above).
- No constructor checks `NewScriptEvent`'s result (it returns NULL when
  `calloc` fails); the field stores go through it unconditionally.

## Extern-type divergences (caller-side levers)

- The goal constructors take `unsigned char flags` first (`mov cl, byte
  ptr [esp+0xc]`), matching levelkw2.c's declarations.
- `AddEvent_Place(void* def, Pos* pos, int count)`, the four rect events,
  `AddEvent_Lookat(Pos*)`, `AddEvent_Gardener_Mechanic(int, int, Pos*)`,
  NEEDAT/NEEDIN/CLEARAREA/STUDAREA take `Pos*` / `Rect*` here; levelkw3.c
  declares the same parameters `int*` — either is fine on the caller's
  side, the callee's copy needs the aggregate (below).
- `g_script_root` is `ScriptEvent*` here (movie3.c: `void*`).

## Levers, with evidence

- **A 16-byte field copy is `e->area = *r` (one aggregate assignment):**
  VC6 emits `lea ecx,[eax+0x28]` then four `mov esi,[edx+k] / mov
  [ecx+k],esi` pairs with the fourth's load hoisted above the flags store,
  exactly the original; the 8-byte `e->pos = *pos` is two register moves.
  Four scalar stores would give four different temporaries.
- **Emitted store order is the source order except across an aggregate
  copy (`AddEvent_Needin`).** Written `f1c = count; elem = elem; area =
  *r;` VC6 emitted elem first (4 mismatches); written `elem = elem; f1c =
  count; area = *r;` it emits f1c first, as the original. The other 70
  constructors kept their source order — the flags store included, which
  is why `AddEvent_Take`/`Addbricks`/`Currency`/`Entrancefee` write
  `flags = 0` BEFORE the argument store and `Give` between its two.
- **Nest the body under the guard for an exiled `return 0`
  (`ShowStepHint`, BL07's PlayMovie shape):** `if (!g_script_cur) return
  0;` keeps `xor eax,eax / ret` inline behind the test (19 mismatches);
  `if (g_script_cur) { … return 1; … return 1; } return 0;` puts it last.
- **Read the parameter order off the frame** (`SetScriptStepText`): the
  step is `[esp+0xc]` after one push and the text `[esp+0x10]` after three,
  so the text is the first argument; the brief's caller-side guess had them
  swapped.
- **`if (step->events) { e->next = …; step->events = e; } else step->events
  = e;`** with the store in both arms is exact for `LinkStepEvent` (two
  stores through different registers); `LinkGoalEvent` has the single
  shared store after the `if`, and both were first-try — spell what the
  original's block count says.
- **`for (step = head; step; step = step->next) { if (…) break; prev =
  step; }`** gives the rotated walk with the null head threaded straight to
  the "no predecessor" arm and `push esi` sunk past it.
- **Inert / not needed**: nothing else; no volatile, no register spelling.

## Verification

```sh
$PY tools/audit.py LEGOLAND/eventmake.c    # 72 x [OK], PASS
$PY tools/relocs.py LEGOLAND/eventmake.c   # 0 MISMATCH, 1 UNRESOLVED ("%s")
ALPHATEAM_VC6_ROOT="$PWD/toolchain" "$LEGOLAND_CL" /nologo /c /W3 /O2 /Gy /Gd /Fo/tmp/sw_w3.obj LEGOLAND/eventmake.c
```
