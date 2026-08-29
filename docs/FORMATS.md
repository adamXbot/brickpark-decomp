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

## Audio / music / video  *(catalogued by `tools/audioinfo.py`, header-verified)*

All counts below were produced by parsing real RIFF headers across
`gamedata/main/` + the mounted disc (`/Volumes/LEGOLAND/`). See
`scratchpad/verify/audio_video/catalog.txt`.

### Speech WAVs — `Speech/*.wav` (1266 files, loose on disc)

**Not plain PCM** (the earlier note was wrong). Every one of the 1266 files is:

| field | value |
| --- | --- |
| wFormatTag | `0x0002` = **Microsoft ADPCM** (4-bit) |
| channels | 1 (mono) |
| sample rate | 22050 Hz |
| bits/sample | 4 |
| nBlockAlign | 512 bytes |
| samplesPerBlock | 1012 |
| chunks | `fmt ` (cbSize 32, with coef table) + `fact` + `data` |

`…z.wav` twins carry the **same** MS-ADPCM format, not a heavier "compressed"
variant — the `z` names mirror in-game object names (`Path`/`Pathz`,
`balloon`/`balloonz`), i.e. a second speech set, not a codec difference.

**Browser playability:** MS-ADPCM is *not* natively decodable by WebAudio
`decodeAudioData`. Either transcode to PCM/Opus at build time, or ship a small
JS MS-ADPCM decoder (the format is trivial: per-block predictor + 4-bit
nibbles, coef table in `fmt `). Cheap either way.

### AVI video — Indeo 5 (40 files total)

| set | codec | size | fps | count | audio |
| --- | --- | --- | --- | --- | --- |
| `AD_*.avi` character anims (`gamedata/main/`) | IV50 | 112×96 | 29.97/30 | 26 | **silent** (video-only) |
| disc-root cutscenes | IV50 | 320×200 (2× 320×240) | 15 | 13 | stereo |
| one legacy cutscene (`California.avi`) | **IV32** (Indeo 3.2) | 320×200 | 15 | 1 | PCM |

Video handler fourcc is `IV50` (Indeo 5, `Ir50_32.dll`) for all but one
`IV32`. `strf` biBitCount = 24. The 26 `AD_*` clips have **no audio stream**.
Cutscene audio streams are mixed: IMA-ADPCM (6), MS-ADPCM (3), PCM (5), all
22050/44100 Hz stereo/mono.

Note: the `avih` dwTotalFrames field is reliable for the `AD_*` clips (16–64
frames) but holds junk for several disc cutscenes; use the per-stream `strh`
dwLength for those.

**Browser playability:** Indeo 5/3 is **not browser-native** and is a
proprietary/legacy codec — must be transcoded (e.g. to H.264/VP9/AV1 MP4/WebM)
at build time. `ffmpeg` decodes IV50/IV32.

### DirectMusic — `.sty` / `.sgt` / `.bnd` / `.bnv`

All are **RIFF DirectMusic forms** except `.bnv`:

| ext | RIFF form fourcc | meaning | count |
| --- | --- | --- | --- |
| `.sty` | `RIFF …DMST` (styh) | DirectMusic **Style** | 212 |
| `.sgt` | `RIFF …DMSG` (segh) | DirectMusic **Segment** | 30 |
| `.bnd` | `RIFF …DMBD` | DirectMusic **Band** | 1 |
| `.bnv` | *non-RIFF*, magic `01 01 40 00 …` | engine-native band/visitor blob (ride visitor sets, e.g. `BlokeBox0N` name table) | 23 |

Styles/segments carry a `UNFO/UNAM` UTF-16 name (e.g. "EItran2", "Band19").
Loaded by `LoadMusicStyle` / `LoadMusicSegment` / `LoadMusicBand` (see
BINARIES.md).

**Browser playability:** DirectMusic dynamic score has **no browser runtime**.
It is effectively out of scope for a faithful port — options are (a) pre-render
the score to audio stems, or (b) reimplement a MIDI-ish sequencer over the
style/segment data. The `.bnv` files are game-logic data, not audio.

### Summary — what plays natively vs needs work

- **Native (with a tiny JS shim):** speech WAVs — MS-ADPCM decode in ~50 lines,
  then WebAudio.
- **Transcode at build time:** all AVI (Indeo 5/3 → MP4/WebM).
- **Out of scope / hard:** DirectMusic `.sty`/`.sgt`/`.bnd` dynamic score.

## Level & object data (from `main.z`)  *(RE in progress)*

10 levels, each with a `.ltx` / `.lms` / `.lfm` triple; tile sets `.tsf` /
`.tsm`; image lists `.ilf`; object definitions `.odf`; `.bnv` (23); plus the
`BUILD MENU` config listing tile sets, mappings and object classes per level.
