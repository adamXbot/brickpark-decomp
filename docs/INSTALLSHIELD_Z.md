# InstallShield 5.x `.z` archive format (`main.z`, `_setup.lib`)

The LEGOLAND disc packs the game program, level data, DirectMusic content and
character-animation AVIs inside `main.z`, with a companion `_setup.lib`. Both
carry signature `0x8C655D13`. This is the **older** InstallShield format — the
one `i5comp`/`i6comp` handle — **not** the `ISc(` cabinet format `unshield`
reads, so it needs its own extractor. Cracked clean-room from the disc bytes
and implemented in [`tools/iscab.py`](../tools/iscab.py) +
[`tools/blast.py`](../tools/blast.py).

## Layout

```
+0x00  u32  signature 0x8C655D13
...          (header)
<member data streams, packed contiguously from low offsets>
<folder table + directory near the tail>
```

The directory is a run of records. Each **file name is described by the 42-byte
metadata block that immediately PRECEDES it**:

```
[42-byte meta describing file 0]
u8 name_len ; char name[name_len]        # file 0
[42-byte meta describing file 1]
u8 name_len ; char name[name_len]        # file 1
...
```

Within a metadata block:

| offset | type | field |
| --- | --- | --- |
| +16 | u32 | uncompressed (expanded) size |
| +20 | u32 | stored (compressed) size |
| +24 | u32 | offset of the DCL stream in the archive |
| +28 | u32 | DOS date/time |

Members pack contiguously (`offset[i] + stored[i] == offset[i+1]`), which is how
the field roles were verified — together with every extracted member's magic
matching its extension (PE `MZ`, RIFF `AVI `/`WAVE`, TrueType, `BM`).

> **The off-by-one tell.** Pair each name with the meta block that *follows* it
> instead and `legoland.exe`'s name lands on the next file's data: an 802 KB PE
> shows up under an AVI's name. The meta block precedes its name.

## Compression: PKWARE DCL "implode"

Every member stream begins `00 06`:

- byte 0 = literal mode (`0` = uncoded 8-bit literals — what LEGOLAND uses).
- byte 1 = dictionary-size exponent (`6` → 4096-byte window).

then a DCL/blast bitstream (LSB-first, length/distance matches with fixed
Huffman tables). Re-implemented in [`tools/blast.py`](../tools/blast.py). Files
that don't compress (the AVIs) come out ~7 % *larger* stored than expanded —
the DCL literal-coding signature, and another confirmation of the field roles.

## Result

`tools/iscab.py extract main.z` yields **331 members** (~14 MB expanded):
`legoland.exe`, `Uninst.dll`, 26 `AD_*.avi` character animations, 212 `.sty` +
30 `.sgt` DirectMusic files, 10 each of `.ltx`/`.lms`/`.lfm` level files, 23
`.bnv`, fonts, and the `BUILD MENU` tile/object config (stored, confusingly, as
`legoland.exe`'s sibling data file). The big media — `Graphics*.res`,
`Legoland.res`, `Speech/*.wav`, the root cutscene AVIs — sit loose on the disc.
