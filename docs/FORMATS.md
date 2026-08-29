# LEGOLAND data formats

Findings from the 2000 USA disc. Confirmed-by-parsing unless marked *(guess)*.

## `.res` — resource archives (`Graphics1.res`, `Graphics2.res`, `Legoland.res`)

A `.res` is a single archive with a **hierarchical name directory at the tail**
and the file data packed from the front.

```
+0x00  u32  offset of the directory
+0x04  ...  member data (each Graphics member is a `COMP` block)
@dir:  tree of folders/files: flags, size, data-offset, NUL-name
```

- `Graphics1.res` (20 MB) / `Graphics2.res` (120 MB): trees of `.lls`
  ("LEGOLAND sprite") images under folders like `Graphics/Icons/…`, each a
  `COMP` block.
- `Legoland.res` (16 MB): starts with floats, not `COMP` — the isometric world
  / model geometry. *(RE in progress.)*

## `COMP` — compressed 16-bit sprite  *(RE in progress)*

Header at each block:

| offset | type | field |
| --- | --- | --- |
| +0 | char[4] | `"COMP"` |
| +4 | u32 | width (e.g. 640) |
| +8 | u32 | height (e.g. 480) |
| +12 | u32 | bit depth (16) |
| +16 | u32 | count / flags (1) |
| +20 | u32 | reserved (0) |
| +24 | u32×4 | sub-stream sizes |
| +40 | … | compressed pixel data (16-bit RGB) |

## Audio / music / video

- **`Speech/*.wav`** — standard PCM WAV, loose on the disc (with `…z.wav`
  compressed twins). Plays directly.
- **`.sty` / `.sgt` / `.bnd`** — DirectMusic styles / segments / bands (the
  dynamic score); `LoadMusicStyle` etc. in the exports.
- **`.avi`** — Indeo-5 video (root cutscenes + `AD_*.avi` character
  animations). Transcode for the browser.

## Level & object data (from `main.z`)  *(RE in progress)*

10 levels, each with a `.ltx` / `.lms` / `.lfm` triple; tile sets `.tsf` /
`.tsm`; image lists `.ilf`; object definitions `.odf`; `.bnv` (23); plus the
`BUILD MENU` config listing tile sets, mappings and object classes per level.
