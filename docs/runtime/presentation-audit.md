# Presentation requirement and evidence audit

Stage 1: inventory the 57 assigned sources, their headers, behavior/bug comments and WIP boundaries; compare them with the presentation page.

Stage 2: consolidate recoverable omissions, reconcile conflicting layouts and annotate only actual source gaps. Output: [presentation specification](presentation.md).

Stage 3: verify source coverage, citations, relative links, required contract terms, source fingerprints and whitespace. The final results and source ledger follow below.

This audit concerns documentation of recovered behavior. It neither compiles the C nor certifies new executable parity. Scope J explicitly requires documentation only. [Scope J](../SCOPE_J_runtime_spec.md)

## Requirements and outcomes

The presentation page follows structures first, then behavior and state machines, decoded tables beside their consumers, original faults, and callback roles linked to the current registration matrix. Every recovered numeric table is transcribed or expressed as an exact generation formula; arrays declared without initializers are explicitly external. [Scope J](../SCOPE_J_runtime_spec.md), [presentation](presentation.md), [callback registrations](callbacks.md)

| Requirement | Evidence and disposition | Result |
| --- | --- | --- |
| Data structures | Sections 1,3,4,5,6: byte offsets/sizes for UI, sprites, LLS/CSP, output lists, popup/orders, pixels and save records; shared full ride records link to transport/attractions | Documented |
| Rules/state machines | Sections 2–6: input, slide states, screens, 1-based report index, popup, frame/raster pipeline, terrain, food service, power | Documented |
| Tables/constants | Button masks, menu/list ordering, price sentinel and decoded prices, screen positions, picker type labels, type3 dispatch, quadrant corrections, bridge offsets, overview terrain switch, heading floats, shade/channel/reciprocal formulas, service actions and unmet-goal codes | Documented; source-only tables enumerated below |
| Original bugs | Section 7 plus local behavior: every explicit bug/uninitialized/leak/unchecked note in the assigned files was checked; newly identified body/header disagreements are separated from memory-safety departures | Documented |
| Callback slots | Section 6 and callbacks.md preserve class/slot/current-name/VA mapping; no duplicated legacy alias matrix | Documented |

The table above records completion of the documentation audit against the [Scope J requirements](../SCOPE_J_runtime_spec.md); it does not convert absent source into known runtime behavior. The [presentation coverage table](presentation.md) retains Partial for five subsystem groups (17 assigned files) with named source boundaries, and Documented for the other 40 files. A Partial label identifies the source boundary; it does not mean the audit or recovered specification was deferred.

## Source-by-source evidence ledger

All 57 assigned files are represented below. “Blocks” counts every C block comment, including field/extern/codegen comments; WIP counts only current `// WIP-FUNCTION` markers. The short SHA-256 fingerprints pin the reviewed source snapshot at integration baseline `f8f5854481b2a87fb456a37b02ce581e9206b400`; source bytes were compared against that commit after rebasing the documentation. Pure compiler-allocation experiments are not runtime rules; their semantic or uncertainty consequences are captured in the WIP ledger. Links in the first column are the evidence for that row. Section numbers refer to [presentation](presentation.md).

| Source | Sections | Recovered scope checked | Blocks | WIP | SHA-256 prefix |
| --- | --- | --- | ---: | ---: | --- |
| [bighelp.c](../../LEGOLAND/bighelp.c) | 1,2,3 | Button edges/repeat; bubble piece order; popup initialization | 173 | 0 | `5265439e4f6f` |
| [bigrender.c](../../LEGOLAND/bigrender.c) | 1,4,5,7 | Scaled versus fatal tiled path; Z stream; cursor footprints, shapes and arrows | 224 | 1 | `e6e83c8fc37b` |
| [bigscreens.c](../../LEGOLAND/bigscreens.c) | 2,3 | Profile/save/progress layouts; interface groups; marker base+8 view | 270 | 0 | `a1ee9ea0b2fe` |
| [fpui.c](../../LEGOLAND/fpui.c) | 1,2,7 | Icon polymorphism; input routing; list sorting; panel timing and NEW byte write | 254 | 1 | `0f21c8a2b2c8` |
| [fpui2.c](../../LEGOLAND/fpui2.c) | 1,2,3,6,7 | Menus/prices/heads; list construction; worker request routing; HUD power | 340 | 0 | `21635031af2a` |
| [fpui3.c](../../LEGOLAND/fpui3.c) | 1,2,3,7 | Slide states and viewport; mission tick; text-cache identity; deletion validation | 157 | 0 | `2d122f9e0336` |
| [fpui4.c](../../LEGOLAND/fpui4.c) | 2,3,5,7 | Flash overlay; pickup duplicates; list leak; horizontal-scroll asymmetry | 140 | 1 | `a043b255b6a8` |
| [fpui5.c](../../LEGOLAND/fpui5.c) | 2,3,7 | 133-price sentinel; work-order cancellation; destructive narration split; NEW shift | 66 | 1 | `59a5386af84e` |
| [gpu.c](../../LEGOLAND/gpu.c) | 4,7 | DirectDraw target stack; keyed blits; Z image scratch; uninitialized fill fields | 140 | 0 | `b14b07221bf8` |
| [iconui.c](../../LEGOLAND/iconui.c) | 1,2 | Icon lifetimes; two lists; help and indicator transitions | 123 | 0 | `77dc2aff7cf4` |
| [input.c](../../LEGOLAND/input.c) | 1,2,7 | Controller units/masks; polling; acceleration; mouse wheel | 73 | 1 | `c329f8dce40a` |
| [input2.c](../../LEGOLAND/input2.c) | 2,7 | Controller defaults; native focus and Backspace; character map; audio invalid-kind return | 140 | 0 | `6b05051eb64c` |
| [layers.c](../../LEGOLAND/layers.c) | 4 | Layer sprite to image holder forwarding | 1 | 0 | `6f0406a9c621` |
| [layervis.c](../../LEGOLAND/layervis.c) | 4 | LLS list/play-once/timer; palette conversion; frame clamps | 27 | 0 | `92e225594984` |
| [mapscreen.c](../../LEGOLAND/mapscreen.c) | 3,5 | Screen-id dispatch; overview readiness/viewport/markers; map click inversion | 106 | 0 | `b1f8256022a2` |
| [mapscreen2.c](../../LEGOLAND/mapscreen2.c) | 3 | 1-based report page; report UI; certificate save counter/path/result | 84 | 0 | `601f880b0da9` |
| [mapscreen3.c](../../LEGOLAND/mapscreen3.c) | 3 | Adverts; tutorial text/click eligibility; park movie transitions | 70 | 0 | `4b6321e88c11` |
| [mapscreen4.c](../../LEGOLAND/mapscreen4.c) | 2,3,7 | Report hint deadlines and empty loop; certificate bindings; drag/arrow input | 96 | 0 | `d362ba943da4` |
| [math3d.c](../../LEGOLAND/math3d.c) | 1,5 | Matrices; fixed depth; eight headings; ascending RenderItem insertion | 50 | 0 | `6a127fa55e6e` |
| [panelui.c](../../LEGOLAND/panelui.c) | 1,2 | Shared Icon layout; menu names; focus/disabled rendering | 46 | 0 | `99a4888c9d64` |
| [popup.c](../../LEGOLAND/popup.c) | 3,4,7 | Details layout; work-order reconciliation; blending/pitch/indeterminate return | 168 | 1 | `bd148f6165b8` |
| [popup2.c](../../LEGOLAND/popup2.c) | 3,7 | Nine slices; footer swap; caption pitch; tools group and disabled inputs | 64 | 0 | `3eca3e9dda68` |
| [powerhelp.c](../../LEGOLAND/powerhelp.c) | 6 | Pool restore/shedding order; salvage and demand invariants | 28 | 0 | `4fc8f0a3bdfd` |
| [printlist.c](../../LEGOLAND/printlist.c) | 1,4 | PrintNode layouts; captured frame/palette; pixel ownership; list reconciliation | 167 | 0 | `65e4836b4a56` |
| [rect.c](../../LEGOLAND/rect.c) | 1 | Inclusive rectangle area accumulation | 1 | 0 | `ffa8fdca6b99` |
| [render2.c](../../LEGOLAND/render2.c) | 4 | Frame order; residency; print tier; staff/seat filters | 70 | 0 | `37ee777f0b17` |
| [render3.c](../../LEGOLAND/render3.c) | 4,5 | CSP stream; RLE byte boundaries; render-node resumption | 94 | 0 | `67eea058192d` |
| [render4.c](../../LEGOLAND/render4.c) | 5,7 | Ground diamond traversal; bridge banks; second-cell wrong coordinates | 60 | 1 | `fc13ea04ac2f` |
| [render5.c](../../LEGOLAND/render5.c) | 1,2,3,4,5 | Cursor tile geometry; vestigial queue; path corners; render-node take; cache generation age; certificate capture | 74 | 0 | `fd148bc74b46` |
| [renderinit.c](../../LEGOLAND/renderinit.c) | 5 | Terrain list binding; bridge-theme decoded offsets; animated bank entries | 43 | 0 | `1619d04c1a6b` |
| [renderlist.c](../../LEGOLAND/renderlist.c) | 1,4 | RenderItem record/arenas; order corrected from linker bodies | 41 | 0 | `b283500cce66` |
| [renderview.c](../../LEGOLAND/renderview.c) | 1,5,7 | Five phases; exact gather/slicing; overview switch table; original leaks | 211 | 2 | `f4e872f0c907` |
| [screen.c](../../LEGOLAND/screen.c) | 3,4 | BMP/LLS; fonts/window setup; class registration forwarded to callbacks index | 422 | 0 | `083844c835b0` |
| [screencb.c](../../LEGOLAND/screencb.c) | 6,7 | Service actions/offsets; resource/dock setup; water query; school removal; raw slide load | 270 | 2 | `f112df88ccc2` |
| [screencb2.c](../../LEGOLAND/screencb2.c) | 6,7 | Resources/layers; save/load restoration; pump/road/school previews; sprite arming bug | 280 | 0 | `3241421e0ad3` |
| [screencb3.c](../../LEGOLAND/screencb3.c) | 6 | EarthSlide POS/RIN patches and draw gates; restaurants; water ghost-chain ordering | 123 | 0 | `415af8c30ac6` |
| [screencb4.c](../../LEGOLAND/screencb4.c) | 6,7 | School ghost rectangles; missing B-to-C link; zebra crossing checks | 62 | 0 | `625aae3c34ff` |
| [screencb5.c](../../LEGOLAND/screencb5.c) | 6,7 | Raw save sizes; entrance teardown; roads preview; ignored river probe | 107 | 0 | `f28b2f52d847` |
| [screencb6.c](../../LEGOLAND/screencb6.c) | 6,7 | Cafe/service removal; sound reference counts; monkey/value/draw descriptors | 60 | 0 | `c0e59e1a9795` |
| [screencb7.c](../../LEGOLAND/screencb7.c) | 6 | Water/tree selection/destruction; power-station audio release | 19 | 0 | `4c2b05d03d1d` |
| [screens2.c](../../LEGOLAND/screens2.c) | 1,3,7 | Title/profile/options geometry; name editors; confirmation popup | 224 | 1 | `ef9f9ae264d6` |
| [screens3.c](../../LEGOLAND/screens3.c) | 2,3,7 | Four-argument input; theme-index mismatch; progress/tutorial/options/advisor | 338 | 0 | `ea159ff7cfeb` |
| [scroll.c](../../LEGOLAND/scroll.c) | 1 | Eight fractional bits; pixel rather than tile scroll units | 3 | 0 | `fc84b73e93b8` |
| [scrolltick.c](../../LEGOLAND/scrolltick.c) | 5,6 | Projected diamond clamp; wear/broken thresholds; power effects | 61 | 0 | `79266390ddb8` |
| [softblit.c](../../LEGOLAND/softblit.c) | 2,4,7 | Raw/type2 recoloring and hit logic; goal dispatch; override asymmetry | 163 | 0 | `3875d64592a5` |
| [softblit2.c](../../LEGOLAND/softblit2.c) | 4 | Type2 two-bit grammar; type3 eight-painter dispatch and external boundary | 80 | 0 | `4f68ead5a0ec` |
| [sprite.c](../../LEGOLAND/sprite.c) | 4 | Surface-pixel residency accessor | 1 | 0 | `0482476a86cb` |
| [sprite2.c](../../LEGOLAND/sprite2.c) | 1,4 | Image/Sprite/ILF ownership, kinds and allocation/residency | 99 | 0 | `240570f4562b` |
| [sprite_override.c](../../LEGOLAND/sprite_override.c) | 4 | Palette override pointer; frame override -1 reset | 3 | 0 | `733edd973a55` |
| [spritemisc.c](../../LEGOLAND/spritemisc.c) | 4 | Reference counts and final-resource teardown | 45 | 0 | `43b9972bb51d` |
| [surface.c](../../LEGOLAND/surface.c) | 4,7 | Lock descriptor; status stack; view output; lost-surface retry | 55 | 0 | `7008fecd2752` |
| [text.c](../../LEGOLAND/text.c) | 1,2,7 | Ten string buckets; GDI layout; speech pieces; singleton leak | 74 | 0 | `09c242ce2bcb` |
| [tilehelp.c](../../LEGOLAND/tilehelp.c) | 1,5 | Tile centers; half offsets; sprite-slot first-fit and flags | 29 | 0 | `6b0a5f0acd63` |
| [tri3d.c](../../LEGOLAND/tri3d.c) | 1,5,7 | Fixed edge/spans; shade/channel tables; unsigned Z; bounds and ownership quirks | 89 | 0 | `3e9b7af9209c` |
| [uimisc.c](../../LEGOLAND/uimisc.c) | 2,3,7 | Help/script lifecycle; report persistence; icon unlink; free-play restore | 142 | 0 | `6e4bde49cc64` |
| [uimisc2.c](../../LEGOLAND/uimisc2.c) | 1,2,3,7 | Help/movie/report state; priority queue; profile restore; icon hit bounds and ownership; free-play/level-end transitions | 218 | 0 | `9053dec1a6e6` |
| [wndenv.c](../../LEGOLAND/wndenv.c) | 2 | Native handle storage/accessors | 3 | 0 | `bef3b6f6559a` |

`misc3.c` is additional cross-subsystem evidence, not an additional assigned file: popup size/mock preview, work-order overlay and LLIDB picker were audited here; its ownership remains in the core inventory. The work-order layout and list insertion reconciliation also cite workorder.c/workorder2.c/workorder3.c. [misc3.c](../../LEGOLAND/misc3.c), [workorder.c](../../LEGOLAND/workorder.c), [workorder2.c](../../LEGOLAND/workorder2.c), [workorder3.c](../../LEGOLAND/workorder3.c)

## WIP boundary ledger

WIP status is the source's executable-matching status, not an automatic documentation omission. The Scope I delta updates measurement evidence without introducing a runtime-behavior change; its compiler experiments were not rerun for Scope J. The recovered behavior of all twelve marked functions is described. The retained distinctions below prevent a reader treating a scheduling residual as an unknown algorithm or a known reconstruction error as shipped behavior. [presentation](presentation.md)

| Function and VA | Source-note boundary | Documentation treatment |
| --- | --- | --- |
| RenderCursor `0x45ff00` | Scratch-register permutation and switch-tail merge threshold | Full footprint, point/color selection, arrow and recursion contract; external segment painters remain unavailable. [bigrender.c](../../LEGOLAND/bigrender.c) |
| InsertChildIntoList `0x475630` | Missing argument-copy instruction; no inferred semantic change | Parent/sibling ordering and absent-parent behavior documented. [fpui.c](../../LEGOLAND/fpui.c) |
| ScrollIconPanel `0x46d850` | Allocation rank residual | Full axis/asymmetric snap and clamp behavior documented from body. [fpui4.c](../../LEGOLAND/fpui4.c) |
| RemoveNewObjectMarker `0x471ca0` | Source/destination induction-pointer anchor | Full shift/clamp/close behavior and adjacent-duplicate skip documented. [fpui5.c](../../LEGOLAND/fpui5.c) |
| UpdateControllerFromMouseData `0x473b00` | Low-clamp constant register | Acceleration and coordinate clamping documented. [input.c](../../LEGOLAND/input.c) |
| DrawPopUpInfo `0x4724a0` | Half-width argument block scheduling | Recovered content, sizes, timing, bar/icons and work-order interpretation documented. [popup.c](../../LEGOLAND/popup.c) |
| PaintTileLayer `0x4608c0` | Nonvolatile half-width home now grouped with tile/dx/dy; remaining head-allocation/frame mismatch | Traversal/overlay rules and wrong second-cell position documented. [render4.c](../../LEGOLAND/render4.c) |
| RenderView `0x45b180` | Two behaviorally unobservable placement discrepancies: x/y loop limits precede quadrant switch; slice-count reset precedes nonnull-object branch; paired correction tested and rejected for matching regression | Original-note placement is documented; Scope I confirms paired testing and no behavioral difference. Current C ordering is not presented as an original bug. [renderview.c](../../LEGOLAND/renderview.c) |
| RenderFullMap `0x4567a0` | ILF address carrier improved at latch; initial count CSE and shared-tail/register/frame mismatches remain | Full scaling, terrain, object, coaster, marker and lifecycle contracts documented. [renderview.c](../../LEGOLAND/renderview.c) |
| BsWater_DrawSelection `0x41bfb0` | Register rotation | Dock redirection, 5×5 cursor and boat occupancy checks documented. [screencb.c](../../LEGOLAND/screencb.c) |
| JcWater_DrawSelection `0x436470` | Same register rotation | Same water-query contract with river globals. [screencb.c](../../LEGOLAND/screencb.c) |
| InitExitCheckBox `0x48f0f0` | Shared zero-register/prologue difference | Confirmation panel behavior and default handlers documented. [screens2.c](../../LEGOLAND/screens2.c) |

## Reconciliations and source-only gaps

The audit resolved the popup “ride” as a work order: target element/name `+4`, assigned `+18`, worker `+1c` and worker action `+60` agree with allocator, assignment and cancel consumers. No recovered C writer populates popup `0x7fdf80` with an order or selects kinds `0x10b/0x10c`; PopUpInfoSetUp has neither case. Therefore payload semantics are established, while entry ownership remains unknown. [popup.c](../../LEGOLAND/popup.c), [fpui5.c](../../LEGOLAND/fpui5.c), [fpui2.c](../../LEGOLAND/fpui2.c), [workorder2.c](../../LEGOLAND/workorder2.c)

Body checks also establish cursor blink as mask `0x100` (512 ms period) and Z sprite codes as two bits (16 codes per dword), correcting contrary header prose. `RenderTiledSprite` terminates with exit(1); only scaled rendering uses the scratch-surface stretch. Type3 A/B/C boundaries agree between dispatchers, but B is described as 16-bit pixels in one header and 8-bit indices in another; absent painters prevent choosing that interpretation. [renderview.c](../../LEGOLAND/renderview.c), [bigrender.c](../../LEGOLAND/bigrender.c), [softblit2.c](../../LEGOLAND/softblit2.c)

| Remaining source boundary | Why it cannot be supplied from this snapshot | Exact supplied contract |
| --- | --- | --- |
| Full free-play price table | Extern rows; headers decode 133+sentinel and four prices only | Row format, case-insensitive lookup, empty-string terminator and 32/27/59/171 decoded prices. [fpui2.c](../../LEGOLAND/fpui2.c), [fpui5.c](../../LEGOLAND/fpui5.c) |
| Keyboard and cheat tables | 59 key pairs and cheat strings are external | Poll/edge/case/ring rules plus decoded special values. [input.c](../../LEGOLAND/input.c), [input2.c](../../LEGOLAND/input2.c) |
| World-marker and interface control rows | Extern tables retain undeclared positions/strings | Record stride, level order, tutorial positions, group/axis positions. [bigscreens.c](../../LEGOLAND/bigscreens.c), [screens3.c](../../LEGOLAND/screens3.c) |
| Type3 specialized painters | Eight plain painters and recoloring frame painter have declarations, no bodies | A/B/C byte offsets, frame/base/override selection, eight-way dispatch geometry and hit arguments; no internal block-B type inferred. [softblit2.c](../../LEGOLAND/softblit2.c), [render3.c](../../LEGOLAND/render3.c), [bigrender.c](../../LEGOLAND/bigrender.c) |
| Cursor segments/colors | DrawCursorSegmentA/B and four color tables are external | Per-point selection, height arguments, footprint tiles and arrow ordering. [bigrender.c](../../LEGOLAND/bigrender.c) |
| Text pixels and asset pixels | Fonts/assets are runtime dependencies rather than decoded source tables | Font sizes/weights, format flags, layouts and clipping. [screen.c](../../LEGOLAND/screen.c), [text.c](../../LEGOLAND/text.c) |

The documentation-completeness claim is limited to recovered facts in this source snapshot. It does not promise undecompiled painter behavior, exact external table values, runtime tests or browser pixel parity. [Scope J](../SCOPE_J_runtime_spec.md), [presentation](presentation.md)

## Integration delta: f22f7cc to f8f5854

The integration baseline added render5.c (six recovered bodies) and uimisc2.c (fourteen). All twenty bodies, their headers, decoded constants and original-bug notes were reviewed, together with the render5/uimisc2 lane write-ups. The seven changed assigned files are bigrender, fpui4, fpui5, popup, render4, renderview and screens2. All source differences in that set were read: Scope I changes only local storage in PaintTileLayer and the address carrier in RenderFullMap; the other edits add matching evidence. Runtime mechanics remain as already specified. [render5.c](../../LEGOLAND/render5.c), [uimisc2.c](../../LEGOLAND/uimisc2.c), [lanes/fable-d-render5.md](../lanes/fable-d-render5.md), [lanes/fable-d-uimisc2.md](../lanes/fable-d-uimisc2.md), [lanes/scope-i.md](../lanes/scope-i.md)

| New body / VA | Closed contract in presentation | Source |
| --- | --- | --- |
| TakeRenderNodeInColumn `0x0045a3e0` | First-live column take, row-before-live-clear order, no-match row zero | [render5.c](../../LEGOLAND/render5.c) |
| FlushCursorSpriteList `0x00461020` | Global replay cursor/count and vestigial shipped list; stale producer comment reconciled | [render5.c](../../LEGOLAND/render5.c) |
| ExpireCachedText `0x00455f70` | Unsigned detail-generation age >10 and compact-without-index-advance | [render5.c](../../LEGOLAND/render5.c) |
| SaveCertificateBitmap `0x00451e20` | CRT timestamp and exact three-argument EGC.bmp saver | [render5.c](../../LEGOLAND/render5.c) |
| DrawPathTileOverlay `0x00460e90` | Base indices code+3..18, corner code+19..21, mode distinction from twin | [render5.c](../../LEGOLAND/render5.c) |
| PaintCursorTiles `0x004610f0` | Caller Pos mutation, two-cell diamond traversal, exact clipping bounds, immediate filtered painter | [render5.c](../../LEGOLAND/render5.c) |
| ReportPrevPageInput `0x00490b90` | Two-argument callback, release step −14, no Next-style disabled forwarding | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| EnqueueObjectHelp `0x00468b00` | Descending priority and destructive head insertion | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| FreeIcon `0x0046d3c0` | Widget ownership and post-free focus/hit pointer comparisons | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| StartFreePlayPark `0x0048abb0` | Fixed database and ordered timer/map/UI transitions | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| SetReportMovie `0x00490610` | 256-byte destination, truncation and duplicate terminator | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| PrintReportLine `0x00491080` | Null guard, fixed 460-wide box and big/small font printers | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| SetScriptEventText `0x00468b40` | Borrow/copy ownership bit and unchecked replacement/allocation | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| RestoreCurrentProfileFromList `0x0048d230` | Saved-to-live field map, reset bytes and no-match behavior | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| KillReportScreenSprites `0x004908b0` | Seven nulling releases then group-7 removal | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| GetIconHitBounds `0x0046de90` | Ordered flag-specific hit-box expansion and widget-extension globals | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| RunLevelEndSequence `0x00459710` | Restored semicolon, pending movie state and missing-separator uninitialized key | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| BlinkReportPageIcons `0x00490ea0` | Inclusive off-hover tests and Next-only blink | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| UpdateHelpBar `0x0046d110` | Four-frame hold-off, 500-ms hover, request lifecycle and three filename formats | [uimisc2.c](../../LEGOLAND/uimisc2.c) |
| PlayMovie `0x004771f0` | Fixed 320×240 target, two prefixes, suppression/return values, audio/render/button lifecycle | [uimisc2.c](../../LEGOLAND/uimisc2.c) |

The new render source closes path-tile composition, cursor tile traversal and column-scan resumption; it does **not** supply DrawCursorSegmentA/B, the eight type-3 plain pixel painters or their recoloring painter. Those unrelated source boundaries remain Partial. The cursor queue's old producer claim is corrected from the lane's executable scan, while the exact path-base expression corrects the header shorthand code+0..15 to code+3..18. [render5.c](../../LEGOLAND/render5.c), [bigrender.c](../../LEGOLAND/bigrender.c), [softblit2.c](../../LEGOLAND/softblit2.c), [lanes/fable-d-render5.md](../lanes/fable-d-render5.md)

The new movie wrapper's header promises all audio layers are restored, but its body resumes only samples/music; it never resumes the streaming track, including after open failure. Help narration has its own target −1 pause-without-resume arm. The specification follows those calls and returns, and retains the int-returning movie ABI despite older void declarations. New names at existing VAs are reconciled locally: PrintSpriteAt/PrintSpriteXY, PrintCursor/small centered text, ResetFrontEnd/PauseCurrentTrack and InitOptionSamples/PauseAllSamples. [uimisc2.c](../../LEGOLAND/uimisc2.c), [tinystubs.c](../../LEGOLAND/tinystubs.c)

Tinystubs is additional borrowed evidence, not a 58th primary file. Its UI/help/popup/render leaves were reviewed and added: default input result, tail linking, clip hook, popup retry/input disabling, guarded state-2 info reset, marker/menu/help clearing, report mode/theme state, cursor validity, empty hooks, two render allocators, detail-image registration and count-owned report buffer freeing. The core audit covers its non-presentation functions. [tinystubs.c](../../LEGOLAND/tinystubs.c), [lanes/scope-e.md](../lanes/scope-e.md)

## Final verification

The final checks operated on the two documentation files and the unchanged source snapshot recorded above. No C, tools or shared index files were changed by this audit, and no compiler, runtime or pixel-comparison test was run.

| Check | Exact method | Outcome |
| --- | --- | --- |
| Assigned-source coverage | Compare the 57 ledger filenames and the introductory presentation-table source links with the presentation inventory; require set equality and 57 unique rows | Passed, 57/57 in each |
| Integration baseline | Compare each of the 57 source files byte-for-byte with `git show f8f5854481b2a87fb456a37b02ce581e9206b400:LEGOLAND/<name>` | Passed, 57/57 |
| Source fingerprints | Compute SHA-256 for each linked source and compare its first 12 hexadecimal characters with the ledger | Passed, 57/57 |
| Comment/WIP inventory | Compare each Blocks count with `source.count('/*')`; compare each WIP count with line-anchored `// WIP-FUNCTION` markers | Passed, 6,571 block comments and 12 WIP markers |
| Relative links | Resolve every Markdown target in both pages against its containing directory, ignoring only URL targets and fragments | Passed, 697 local links and no missing targets |
| Narrative citations | Check each presentation prose paragraph longer than 100 characters, excluding headings, tables and fenced code, for a Markdown citation | Passed, no uncited factual narrative paragraphs |
| Required recovered contracts | Check the page contains the 1-based report convention, 32n popup width, work-order kind 0x10b, RLEPaintHitClipLR dispatch, SHARK CAFE service, 512 ms blink period, two-bit Z controls, exit(1) tiled stub, 0x24-byte slide save and exact work-order edge expression | Passed; reviewed against the cited function bodies |
| Coverage vocabulary | Require Partial for the five groups with material source gaps; distinguish WIP executable-matching residuals from unknown runtime behavior | Passed, 17 Partial / 40 Documented assigned sources |
| Placeholders | Search both pages for TODO, TBD, FIXME, placeholder, pending audit or audit-to-follow language | Passed, none |
| Whitespace | `git diff --check -- docs/runtime/presentation.md docs/runtime/presentation-audit.md` | Passed |

These checks establish source traceability, inventory completeness and consistency of this documentation. The WIP and source-boundary ledgers above remain the limits on recovered runtime knowledge.
