# Scope J — the runtime spec (2026-09-05)

**Read `docs/PARALLEL_CONTRACT.md` first** for the project, the environment
and the git rules. Branch: `scope/J`. This scope **touches no `.c` file and
runs no compiler**; it produces documentation only, so it cannot collide with
anything.

## The job

The project's stated goal is a matching decompilation *and eventually a
browser runtime*, and the recovered mechanics in the file headers and the
notes above functions are that runtime's spec — but they are scattered across
~240 files, written by dozens of sessions, in dozens of styles. Consolidate
them into **`docs/RUNTIME_SPEC.md`**: one document, organised by subsystem,
that a runtime author could implement from without reading the C.

## What goes in

For each subsystem, in this order:

1. **Data structures** — record layouts with offsets, sizes and field
   meanings, exactly as the headers state them (e.g. `BsBoat` 0x3f4 bytes,
   head 0x004cc03c; `TowerRec` car slots at +0x14..+0xa4, 36 bytes each;
   `TrackJoint` `{dir, float h, node}`; the RK4 solver descriptor and its
   ten-entry vector-op table; the `.csp` composite-sprite file format; the
   save chunk formats; the z-buffer command `{int n; short ylast; short;
   {short x; short y; int step;} v[n]}`).
2. **Rules and state machines** — the mechanics in prose: the log-flume boat
   timeline (`t = drop-step + z`, the 120-frame ramp, the splash), the
   boating-school reservation mask and direction preference, the school-car
   manoeuvre codes (0 stop, 1 left, 2 straight, 3 right, 4 pull off, 5
   arrive) and the road graph's one-way entrance, the coaster's lap state
   machine (`8 → 4 → 0x10`) and its 0.8 s clamp and bisection landing, the
   space tower's derived "in service", the report screen's 1-based line
   index, the side panel's slide states, visitor tiredness thresholds
   `{1000, 2400, 4000, 7000}`, the gold-rush pan drift, and so on.
3. **Tables and constants** — every `.rdata` table the headers decode
   (headings, offsets, slot tables, the sixteen-point heading, the pan slots,
   the queue spots, the shade ramps, the free-play table's terminator rule).
4. **Original bugs** — every "original bug reproduced" note, stated as
   behaviour the runtime must preserve (or may consciously fix, but must
   know about): the uninitialised returns, the null-head unlinks, the swapped
   footer corners, the caption-width pitch error, the sixteenth waypoint
   shift reading past the array, `LFCorner_Place` reading `parent->sq` before
   its null check, the joust freeze, the never-armed Balloonz sprite, the
   unbounded queue-spot index, and the rest.
5. **Callback slots** — fold `docs/RIDE_CALLBACKS.md` in: the ObjDef slot
   meanings (8c, 90, 94, 98, 9c, a0, a4, a8, ac, b0, b8, bc) and which class
   fills each, with the function names as they now stand.

## Where the material is

- Every `LEGOLAND/*.c` file header (the `/* ... */` block at the top) and
  the notes above `// WIP-FUNCTION:` markers.
- `docs/lanes/*.md` — the parallel sessions' notes, each with a "Mechanics
  recovered" section.
- `docs/RIDE_CALLBACKS.md`, `docs/FORMATS.md`, `docs/BINARIES.md`,
  `docs/RE_CONTEXT.md`, `docs/INSTALLSHIELD_Z.md` — existing reference docs
  to fold in or link.
- `docs/DECOMP.md` sections "LoadBaseMap interface", "LLIDB image database",
  "Global names from the export table".

Cite the source file for every fact (`[bswater2.c]`, `[lanes/fable-c.md]`)
so a reader can go to the C. Where two files disagree (it happens —
`logflume.c`'s `LFAnimRefs` vs `logflume4.c`'s `LFQueue`, later reconciled as
`{path, head, tail}`; `TrackJoint`'s +0x00 as height vs direction), state the
reconciled version and note the disagreement.

## Rules

- **Do not edit any `.c` file, `tools/`, `docs/DECOMP.md`, `docs/HANDOFF.md`,
  `README.md` or the progress report.** Create `docs/RUNTIME_SPEC.md` and, if
  useful, `docs/runtime/` for per-subsystem pages; commit on `scope/J`.
- Do not invent: if a header is ambiguous, say so rather than guessing.
- Do not paraphrase the C into pseudo-code; describe behaviour, layouts and
  constants.
- Keep a **coverage table** at the top: subsystem → source files → status
  (documented / partial / not yet), so the gaps are visible.

## Report format

In your final message: the document's section list with sizes, the coverage
table, the disagreements you found between files, and anything you could not
resolve from the sources.
