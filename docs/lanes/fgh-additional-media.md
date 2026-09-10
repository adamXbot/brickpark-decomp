# Additional LEGOLAND media review — 2026-09-07

The user supplied six additional disc/archive copies, a CUE, the alternate
installer, an Indeo codec folder and a German disc photograph. Static inspection
found **one further executable build, a useful reissue compatibility setup and
additional localization material. The eight remaining F/G/H bodies have no
alternate compiler arrangements; the 40/48 checkpoint is unchanged.**

Full local report and hashes: `scratchpad/fgh/backup-review/additional-copies/README.md`.
Its JSON manifests identify every input and disc member. This supplements the
earlier English/Dutch investigation in `scratchpad/fgh/backup-review/README.md`.

| Supplied copy | Finding |
| --- | --- |
| `LEGOLAND-2.bin` / `LEGOLAND-2.cue` | German edition. Game EXE is byte-identical to the earlier Dutch build. CUE incorrectly names `LEGOLAND.bin`; it should name `LEGOLAND-2.bin` when beside the new image. Source CUE unchanged. |
| `LEGOLAND-3.iso` | Spanish edition; same game EXE as German and Dutch. |
| `LEGOLAND-4.iso` | English Focus Multimedia reissue. Original English EXE, compatibility database, pre-extracted installation tree and a different UK manual. |
| `LEGOLAND.iso` | Byte-identical to the English ISO already found in both earlier ZIPs. |
| `Legoland-2.iso` | Czech edition with a distinct June 2001 executable and CPack installation archive. |
| `Lego Land for Internet Archive.rar` | Only the known English ISO and Alternate Installer 1.0; both match earlier copies. |
| Standalone Alternate Installer 1.0 | Same hash as previously reviewed; no new game build. |
| `INDEO_IV50_Codec/` | Loose DLL reports 5.10; installer reports 5.11. Useful playback references, no game source. |
| `1.jpg` | German-language disc artwork, identifier `IB2G-LANGE5.2`; packaging evidence rather than proof of image provenance. |

- **New Czech build:** 815,204 bytes, PE timestamp 2001-06-11 09:26:45 UTC,
  713 exports, SHA-256
  `8b4f8c2f800046d07b71fe1c6f8e22c1853152c5b313c13a032f021b08aa8557`.
  It still reports `V.0.229`, so version labels alone are insufficient. Its
  CodeView path is
  `C:\CPP\lokalizace\lland_cz\kit\legoland\legoland___Win32_Release_ROXXE\legoland.pdb`
  (age 10). This is a useful localization-kit clue; the PDB is not present.
  Header timestamps are not independently verified release dates.
- **Eight-target comparison:** each target has a unique Czech signature hit,
  independently bounded with Czech export addresses. All eight match English
  normalized instructions, widths, complete sizes, internal branch offsets
  and applicable jump-table offsets. Their start addresses also match English.
  Absolute references and external callees were normalized; semantic identity
  and runtime behavior were not fully tested. German/Spanish identity with
  Dutch carries over the earlier eight-target structural result.
- **Broader Czech differences:** of 672 shared code exports, 666 have the same
  normalized sequence and size. The six different bodies are `GetVisitorName`,
  `InitListProfiles`, `InsertChildIntoList`, `LLIDB_LoadICM`, `RES_OpenFile`, and
  `StoreNewSaveGameToDisk`. This is not a complete census of unexported changes.
- **Focus compatibility:** installer metadata names Focus Multimedia Ltd and
  schedules installation of a 584-byte `legoland.sdb` on NT 6.0 and later.
  The embedded and root database copies match; strings include `WinXPSp2`,
  `256Color`, and `RunAsAdmin`. All 331 English installation-archive files,
  all 1,266 speech files and the root game-resource archives match English.
  Thus this is a concrete compatibility reference without a replacement game
  EXE. Settings were inspected, not applied or validated on current Windows.
- **UK manual:** 23 PDF pages, including facing-page spreads through numbered
  page 43, differs from the earlier 36-page PDF. Covers and printed pages 26–29
  were visually reviewed. Useful visitor-query, repair-cost, broken-ride scrap
  and insufficient-power behavior reference; no claim of new gameplay.
- **Localization assets:** German/Spanish installation archives each preserve
  327/331 English members; Czech preserves 326, changes four and omits Uninst.dll.
  Text, names, artwork and speech differ. All 17 recovered map records remain
  byte-identical to English in German, Spanish and Czech. Resource counts use
  the existing leaf parser and retain duplicate names; no new levels found.
- **Czech extraction:** statically recovered all 330 `data/lego.pak` members
  using raw DEFLATE with per-member size and CRC32 validation. The final record
  ends at the archive end. `data/lland.pak` overlaps every other disc file and
  is not another independent 448 MB payload. The `.tab` files are PE programs,
  not plain text tables, and were not run.
- **Codec distinction:** the supplied loose DLL is 5.10, while the English
  disc's `MYIr50_32.dll` reports 5.11 and Czech's DLL reports 5.11.15.2.56.
  The supplied readme's installation steps are document content, not executed
  instructions. Codec behavior remains untested.
- **No usable debug/source payload:** PDB references are paths only;
  `ROLLERCOASTER.obj` is the known 116-byte text model-component list and
  `vssver.scc` is resource-side source-control metadata. No source/PDB recovered.

All supplied media and the project's English reference remain untouched.
No supplied executable was run and no game C or matching gate changed. The user
subsequently authorized committing and pushing the research notes, analysis
scripts and JSON metadata. Game binaries, assets, manuals, extracted readmes
and paired disassembly listings remain local and excluded from publication.

## Subsequent Japanese demo — 2026-09-07

`/Users/systemadmin/Downloads/LegoLandDemo.exe` was statically unpacked into
732 game files. It contains a **fourth distinct game executable family**:
815,168 bytes, PE timestamp 2001-01-31 06:28:45 UTC, 713 exports, SHA-256
`31f54cb742518a494df1683a67e3da6ca44c0d7329d123a682261d252d489e73`.
It still reports `V.0.229`. The CodeView path is
`C:\MyProject\LegoLand\DEMO_Release\legoland.pdb` (age 2), without a PDB payload.
Japanese installer metadata and readme confirm the trial edition.

All eight F/G/H targets again have the same normalized instructions, sizes,
widths, internal branches and applicable jump-table offsets as English, now
also at the same start addresses. No alternate compiler arrangement or matching
promotion resulted. Of 672 shared code exports, 660 match normalized sequence
and size; twelve differ around profile/save entry, names, screens, ICM loading
and the window procedure. Added IME imports offer a Japanese text-input research
lead. No source or usable symbols were recovered. All 17 recovered maps match
English, including retail maps; their presence does not establish demo access.

Full local report, hashes, inventories and paired comparisons:
`scratchpad/fgh/backup-review/japanese-demo/README.md`. All 732 extracted sizes
were checked against the cabinet listing. Input and reference hashes remain
unchanged. No supplied executable was run. Publication follows the same scope
as the earlier reviews: authored notes, analysis scripts and JSON metadata.
