# Source coverage

Inventory: all192 tracked `LEGOLAND/*.c` files at baseline `f8f5854481b2a87fb456a37b02ce581e9206b400`. Each source has exactly one primary page below; cross-citations intentionally overlap. “Documented” covers the recovered contract, not original-binary verification or a finished runtime. “Partial” means known material is consolidated but a specific data or behavioral boundary remains. No source is “not yet” assigned. [Scope brief](../SCOPE_J_runtime_spec.md)

## World

Primary page: [world.md](world.md). 36 source files; 28 documented, 8 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [bigsim.c](../../LEGOLAND/bigsim.c) | partial | Simulation/category contracts recovered; low-level Bloke dispatch handlers include unrecovered state7 completion. |
| [blokeai.c](../../LEGOLAND/blokeai.c) | partial | All26 high-level slots identified; low-level table and several high-level handler bodies remain external. |
| [blokelist.c](../../LEGOLAND/blokelist.c) | partial | Appearance and lifecycle recovered; first-name and surname table contents remain external. |
| [blokemisc.c](../../LEGOLAND/blokemisc.c) | partial | Departure-score ring recovered; numeric score bucket thresholds remain external. |
| [bnvmove.c](../../LEGOLAND/bnvmove.c) | partial | MoveLine input units conflict with ride callers; low-level arrival/state7 completion body remains absent. |
| [buildtick.c](../../LEGOLAND/buildtick.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [loadmap.c](../../LEGOLAND/loadmap.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [map.c](../../LEGOLAND/map.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [mapbuild.c](../../LEGOLAND/mapbuild.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [mapinit.c](../../LEGOLAND/mapinit.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [mapobj.c](../../LEGOLAND/mapobj.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [mappath.c](../../LEGOLAND/mappath.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [maprestore.c](../../LEGOLAND/maprestore.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [misc3.c](../../LEGOLAND/misc3.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [money.c](../../LEGOLAND/money.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [objdoor.c](../../LEGOLAND/objdoor.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [objmap.c](../../LEGOLAND/objmap.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [objmap2.c](../../LEGOLAND/objmap2.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [objrect.c](../../LEGOLAND/objrect.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathbuild.c](../../LEGOLAND/pathbuild.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathgfx.c](../../LEGOLAND/pathgfx.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathmisc.c](../../LEGOLAND/pathmisc.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathmisc2.c](../../LEGOLAND/pathmisc2.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathsq.c](../../LEGOLAND/pathsq.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [pathtile2.c](../../LEGOLAND/pathtile2.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [posstep.c](../../LEGOLAND/posstep.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [power.c](../../LEGOLAND/power.c) | partial | Pool rules recovered; full65-entry name/power table is external, with only examples decoded. |
| [simcore.c](../../LEGOLAND/simcore.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [simcore2.c](../../LEGOLAND/simcore2.c) | partial | Visitor thresholds and state transitions recovered; Bloke+7c physiological meaning conflicts with food/ride callers. |
| [workers.c](../../LEGOLAND/workers.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [workers2.c](../../LEGOLAND/workers2.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [workers3.c](../../LEGOLAND/workers3.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [workorder.c](../../LEGOLAND/workorder.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [workorder2.c](../../LEGOLAND/workorder2.c) | partial | Work-order rules recovered; full17-entry message cooldown values and ordered nearby-cell offsets remain external. |
| [workorder3.c](../../LEGOLAND/workorder3.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |
| [workorder4.c](../../LEGOLAND/workorder4.c) | documented | Recovered layouts, placement/path/staff rules, decoded tables and faults consolidated. |

## Persistence

Primary page: [persistence.md](persistence.md). 6 source files; 0 documented, 6 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [profiles.c](../../LEGOLAND/profiles.c) | partial | Packed profile layout recovered; thirty-name-bytes-plus-flags versus32-name-bytes remains unresolved. |
| [savechunks.c](../../LEGOLAND/savechunks.c) | partial | Worker/order/script payloads mapped; script writer framing and some raw field meanings remain unknown. |
| [savechunks2.c](../../LEGOLAND/savechunks2.c) | partial | Event-list format recovered; external string writer prevents resolving the reader/writer framing conflict. |
| [savegame.c](../../LEGOLAND/savegame.c) | partial | Container and flattened visitor fields mapped; meanings of selected raw saved fields remain unknown. |
| [savegame2.c](../../LEGOLAND/savegame2.c) | partial | Path/build/terrain and string reader recovered; string writer framing remains external. |
| [saveprof.c](../../LEGOLAND/saveprof.c) | partial | Profile block+43 corrected; name/flag region interpretation remains unresolved. |

## Assets

Primary page: [assets.md](assets.md). 32 source files; 25 documented, 7 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [anim2.c](../../LEGOLAND/anim2.c) | partial | Texture remap and transport behavior recovered; referenced boat artwork/seat table values remain external. |
| [audio2.c](../../LEGOLAND/audio2.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [audio3.c](../../LEGOLAND/audio3.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [audio4.c](../../LEGOLAND/audio4.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [audio5.c](../../LEGOLAND/audio5.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [audiomisc.c](../../LEGOLAND/audiomisc.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [blokeanim.c](../../LEGOLAND/blokeanim.c) | partial | Frame-control rules recovered; animation/model table contents depend on external data. |
| [bnvpath.c](../../LEGOLAND/bnvpath.c) | partial | Path/orientation behavior recovered; unused bytes in20-byte BNV vertices have no established meaning. |
| [data2.c](../../LEGOLAND/data2.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [data3.c](../../LEGOLAND/data3.c) | partial | LOC texture/context fixups recovered; complete texture-name capacity and opaque context fields remain unknown. |
| [lifecycle.c](../../LEGOLAND/lifecycle.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [listdel.c](../../LEGOLAND/listdel.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [llidb.c](../../LEGOLAND/llidb.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [llidb_load.c](../../LEGOLAND/llidb_load.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [llidb_odf.c](../../LEGOLAND/llidb_odf.c) | partial | ODF load and registration order recovered; finalizer and normal-path initializer ownership remain external. |
| [loaders.c](../../LEGOLAND/loaders.c) | partial | LoadPos records mapped; first three raw scalars are not interpreted by the loader and asset variants conflict. |
| [memdb.c](../../LEGOLAND/memdb.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [music.c](../../LEGOLAND/music.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [person3d.c](../../LEGOLAND/person3d.c) | partial | Morph/person rendering contract recovered; some person/model field meanings and external assets remain unknown. |
| [res.c](../../LEGOLAND/res.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [rin.c](../../LEGOLAND/rin.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sweep1.c](../../LEGOLAND/sweep1.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sweep2.c](../../LEGOLAND/sweep2.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sweep3.c](../../LEGOLAND/sweep3.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sweep4.c](../../LEGOLAND/sweep4.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sweep5.c](../../LEGOLAND/sweep5.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sysmisc.c](../../LEGOLAND/sysmisc.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sysmisc2.c](../../LEGOLAND/sysmisc2.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sysmisc3.c](../../LEGOLAND/sysmisc3.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [sysstubs.c](../../LEGOLAND/sysstubs.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [tinystubs.c](../../LEGOLAND/tinystubs.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |
| [util.c](../../LEGOLAND/util.c) | documented | Recovered loading/format/lifecycle contracts consolidated. |

## Transport

Primary page: [transport.md](transport.md). 32 source files; 18 documented, 14 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [bswater.c](../../LEGOLAND/bswater.c) | partial | Partial: 16×25 water artwork table is external, not decoded |
| [bswater2.c](../../LEGOLAND/bswater2.c) | documented | Documented |
| [bswater3.c](../../LEGOLAND/bswater3.c) | partial | Partial: raw boat direction/arc table rows are external |
| [coaster.c](../../LEGOLAND/coaster.c) | partial | Partial: allocated piece tail and some class descriptors unnamed |
| [coaster3d.c](../../LEGOLAND/coaster3d.c) | partial | Partial: full model templates not decoded |
| [coaster4.c](../../LEGOLAND/coaster4.c) | partial | Partial: external model tables; draw-order direction pairs decoded |
| [coaster5.c](../../LEGOLAND/coaster5.c) | partial | Partial: recovered rules documented; class masks remain external |
| [coaster6.c](../../LEGOLAND/coaster6.c) | documented | Documented recovered rules; asset substitutions depend on model data |
| [coaster7.c](../../LEGOLAND/coaster7.c) | documented | Documented recovered contracts |
| [coastermath.c](../../LEGOLAND/coastermath.c) | documented | Documented recovered interfaces and edge behaviour |
| [coastertiny.c](../../LEGOLAND/coastertiny.c) | partial | Partial: cursor-mode and support-template values remain external |
| [jcroute.c](../../LEGOLAND/jcroute.c) | documented | Documented |
| [junglecruise.c](../../LEGOLAND/junglecruise.c) | partial | Partial: artwork external; decoration removal views recovered, larger allocation unknown |
| [lfentrance.c](../../LEGOLAND/lfentrance.c) | documented | Documented |
| [lfmisc.c](../../LEGOLAND/lfmisc.c) | documented | Documented |
| [logflume.c](../../LEGOLAND/logflume.c) | partial | Partial: external overlay offsets and image tables |
| [logflume2.c](../../LEGOLAND/logflume2.c) | partial | Partial: class geometry rectangles are asset-derived |
| [logflume3.c](../../LEGOLAND/logflume3.c) | documented | Documented recovered floor plans; dimensions follow ODF footprints |
| [logflume4.c](../../LEGOLAND/logflume4.c) | documented | Documented with corrected queue/splash/path interpretation |
| [logflume5.c](../../LEGOLAND/logflume5.c) | documented | Documented |
| [logflume6.c](../../LEGOLAND/logflume6.c) | documented | Documented with exact-endpoint caveat |
| [logflume7.c](../../LEGOLAND/logflume7.c) | documented | Documented; endpoint safety claim unresolved |
| [roads.c](../../LEGOLAND/roads.c) | partial | Partial: road table decoded; raw boat arc table rows external |
| [roads2.c](../../LEGOLAND/roads2.c) | documented | Documented |
| [schoolcar.c](../../LEGOLAND/schoolcar.c) | partial | Partial: model assets and remaining unnamed route fields |
| [schoolcar2.c](../../LEGOLAND/schoolcar2.c) | documented | Documented |
| [schoolcar3.c](../../LEGOLAND/schoolcar3.c) | documented | Documented recovered pipeline; original matching residuals remain |
| [schoolcar4.c](../../LEGOLAND/schoolcar4.c) | documented | Documented |
| [schoolcar5.c](../../LEGOLAND/schoolcar5.c) | documented | Documented |
| [schoolcar6.c](../../LEGOLAND/schoolcar6.c) | documented | Documented |
| [schoolcar7.c](../../LEGOLAND/schoolcar7.c) | partial | Partial: LMS payload schema remains opaque |
| [schoolcar8.c](../../LEGOLAND/schoolcar8.c) | partial | Partial: recovered helpers documented; three seat-x values/spacing remain external |

## Attractions

Primary page: [attractions.md](attractions.md). 29 source files; 18 documented, 11 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [castleobj.c](../../LEGOLAND/castleobj.c) | documented | Documented adapter/layout and recovered geometry across this page and [transport.md](transport.md#5-coaster-graph-physics-rendering-and-save) |
| [catapult.c](../../LEGOLAND/catapult.c) | partial | Partial: mechanics recovered; external layer/landing table values absent |
| [goldrush.c](../../LEGOLAND/goldrush.c) | documented | Documented recovered scripts/tables; pan-offset arithmetic reconciled below |
| [goldrush2.c](../../LEGOLAND/goldrush2.c) | documented | Documented recovered scripts/tables; pan-offset arithmetic reconciled below |
| [goldrush3.c](../../LEGOLAND/goldrush3.c) | documented | Documented recovered scripts/tables; pan-offset arithmetic reconciled below |
| [goldrush4.c](../../LEGOLAND/goldrush4.c) | documented | Documented recovered scripts/tables; pan-offset arithmetic reconciled below |
| [interfaces.c](../../LEGOLAND/interfaces.c) | documented | Documented provider roles and formats; stale field names reconciled below |
| [joust.c](../../LEGOLAND/joust.c) | partial | Partial: recovered rider/cycle mechanics documented; Temple Slide walk/end threshold bytes remain external |
| [joust2.c](../../LEGOLAND/joust2.c) | partial | Partial: recovered rider/cycle mechanics documented; Temple Slide walk/end threshold bytes remain external |
| [mechrides.c](../../LEGOLAND/mechrides.c) | partial | Partial: rider machines and layouts recovered; several vehicle-machine helpers remain externs |
| [ridecb1.c](../../LEGOLAND/ridecb1.c) | partial | Partial: missing contents of cafe/waiter and some seat tables |
| [ridecb2.c](../../LEGOLAND/ridecb2.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridecb3.c](../../LEGOLAND/ridecb3.c) | partial | Partial: missing contents of cafe/waiter and some seat tables |
| [ridecb4.c](../../LEGOLAND/ridecb4.c) | partial | Partial: missing contents of cafe/waiter and some seat tables |
| [ridecb5.c](../../LEGOLAND/ridecb5.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridecb6.c](../../LEGOLAND/ridecb6.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridecb7.c](../../LEGOLAND/ridecb7.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridecb8.c](../../LEGOLAND/ridecb8.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridecb9.c](../../LEGOLAND/ridecb9.c) | documented | Documented recovered callback contracts, faults and [transport routes/vehicles](transport.md); boating queue coordinates remain external |
| [ridemisc.c](../../LEGOLAND/ridemisc.c) | partial | Documented for recovered contracts; partial external animation tables |
| [ridemisc2.c](../../LEGOLAND/ridemisc2.c) | partial | Documented for recovered contracts; partial external animation tables |
| [ridemisc3.c](../../LEGOLAND/ridemisc3.c) | partial | Documented for recovered contracts; partial external animation tables |
| [ridemisc4.c](../../LEGOLAND/ridemisc4.c) | documented | Documented 77 recovered helper bodies; seven machine-step bodies and Tower seat picker remain external |
| [rides.c](../../LEGOLAND/rides.c) | partial | Documented for recovered contracts; partial external animation tables |
| [ridesave.c](../../LEGOLAND/ridesave.c) | documented | Documented provider roles and formats; stale field names reconciled below |
| [ridetiny.c](../../LEGOLAND/ridetiny.c) | documented | Documented 77 recovered helper bodies; seven machine-step bodies and Tower seat picker remain external |
| [waterworks.c](../../LEGOLAND/waterworks.c) | documented | Documented; garden fallback prose reconciled against callbacks |
| [westtown.c](../../LEGOLAND/westtown.c) | documented | Documented scripts, door state and overlay ordering |
| [westtown2.c](../../LEGOLAND/westtown2.c) | documented | Documented scripts, door state and overlay ordering |

## Presentation

Primary page: [presentation.md](presentation.md). 57 source files; 40 documented, 17 partial.

| Source file | Status | Boundary |
| --- | --- | --- |
| [bighelp.c](../../LEGOLAND/bighelp.c) | partial | Partial; source gap: full 59-entry character map and cheat strings remain external |
| [bigrender.c](../../LEGOLAND/bigrender.c) | partial | Partial; source gaps: external type-3 painters and cursor-segment pixel shapes |
| [bigscreens.c](../../LEGOLAND/bigscreens.c) | partial | Partial; source gap: marker and control-position extern tables remain incomplete |
| [fpui.c](../../LEGOLAND/fpui.c) | documented | Documented |
| [fpui2.c](../../LEGOLAND/fpui2.c) | partial | Partial; source gap: the full 133-price table is not decoded |
| [fpui3.c](../../LEGOLAND/fpui3.c) | partial | Partial; source gap: the full 133-price table is not decoded |
| [fpui4.c](../../LEGOLAND/fpui4.c) | partial | Partial; source gap: the full 133-price table is not decoded |
| [fpui5.c](../../LEGOLAND/fpui5.c) | partial | Partial; source gap: the full 133-price table is not decoded |
| [gpu.c](../../LEGOLAND/gpu.c) | documented | Documented |
| [iconui.c](../../LEGOLAND/iconui.c) | documented | Documented |
| [input.c](../../LEGOLAND/input.c) | partial | Partial; source gap: full 59-entry character map and cheat strings remain external |
| [input2.c](../../LEGOLAND/input2.c) | partial | Partial; source gap: full 59-entry character map and cheat strings remain external |
| [layers.c](../../LEGOLAND/layers.c) | documented | Documented |
| [layervis.c](../../LEGOLAND/layervis.c) | documented | Documented |
| [mapscreen.c](../../LEGOLAND/mapscreen.c) | documented | Documented |
| [mapscreen2.c](../../LEGOLAND/mapscreen2.c) | documented | Documented |
| [mapscreen3.c](../../LEGOLAND/mapscreen3.c) | documented | Documented |
| [mapscreen4.c](../../LEGOLAND/mapscreen4.c) | documented | Documented |
| [math3d.c](../../LEGOLAND/math3d.c) | documented | Documented with corrected scroll-unit and sort-order descriptions |
| [panelui.c](../../LEGOLAND/panelui.c) | documented | Documented |
| [popup.c](../../LEGOLAND/popup.c) | partial | Partial; ride/work-order labels reconciled; popup-entry producer remains absent |
| [popup2.c](../../LEGOLAND/popup2.c) | partial | Partial; ride/work-order labels reconciled; popup-entry producer remains absent |
| [powerhelp.c](../../LEGOLAND/powerhelp.c) | documented | Documented |
| [printlist.c](../../LEGOLAND/printlist.c) | documented | Documented with corrected list interpretation |
| [rect.c](../../LEGOLAND/rect.c) | documented | Documented with corrected scroll-unit and sort-order descriptions |
| [render2.c](../../LEGOLAND/render2.c) | documented | Documented |
| [render3.c](../../LEGOLAND/render3.c) | partial | Partial; source gaps: external type-3 painters and cursor-segment pixel shapes |
| [render4.c](../../LEGOLAND/render4.c) | documented | Documented; explicit WIP boundaries in the audit |
| [render5.c](../../LEGOLAND/render5.c) | documented | Documented; explicit WIP boundaries in the audit |
| [renderinit.c](../../LEGOLAND/renderinit.c) | documented | Documented; explicit WIP boundaries in the audit |
| [renderlist.c](../../LEGOLAND/renderlist.c) | documented | Documented with corrected list interpretation |
| [renderview.c](../../LEGOLAND/renderview.c) | documented | Documented; explicit WIP boundaries in the audit |
| [screen.c](../../LEGOLAND/screen.c) | documented | Documented |
| [screencb.c](../../LEGOLAND/screencb.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb2.c](../../LEGOLAND/screencb2.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb3.c](../../LEGOLAND/screencb3.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb4.c](../../LEGOLAND/screencb4.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb5.c](../../LEGOLAND/screencb5.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb6.c](../../LEGOLAND/screencb6.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screencb7.c](../../LEGOLAND/screencb7.c) | documented | Documented; [attractions](attractions.md) and [transport](transport.md) supply shared ride mechanics |
| [screens2.c](../../LEGOLAND/screens2.c) | partial | Partial; source gap: marker and control-position extern tables remain incomplete |
| [screens3.c](../../LEGOLAND/screens3.c) | partial | Partial; source gap: marker and control-position extern tables remain incomplete |
| [scroll.c](../../LEGOLAND/scroll.c) | documented | Documented with corrected scroll-unit and sort-order descriptions |
| [scrolltick.c](../../LEGOLAND/scrolltick.c) | documented | Documented with corrected scroll-unit and sort-order descriptions |
| [softblit.c](../../LEGOLAND/softblit.c) | partial | Partial; source gaps: external type-3 painters and cursor-segment pixel shapes |
| [softblit2.c](../../LEGOLAND/softblit2.c) | partial | Partial; source gaps: external type-3 painters and cursor-segment pixel shapes |
| [sprite.c](../../LEGOLAND/sprite.c) | documented | Documented |
| [sprite2.c](../../LEGOLAND/sprite2.c) | documented | Documented |
| [sprite_override.c](../../LEGOLAND/sprite_override.c) | documented | Documented |
| [spritemisc.c](../../LEGOLAND/spritemisc.c) | documented | Documented |
| [surface.c](../../LEGOLAND/surface.c) | documented | Documented |
| [text.c](../../LEGOLAND/text.c) | documented | Documented |
| [tilehelp.c](../../LEGOLAND/tilehelp.c) | documented | Documented with corrected scroll-unit and sort-order descriptions |
| [tri3d.c](../../LEGOLAND/tri3d.c) | documented | Documented with corrected ramp allocation layout |
| [uimisc.c](../../LEGOLAND/uimisc.c) | documented | Documented |
| [uimisc2.c](../../LEGOLAND/uimisc2.c) | documented | Documented |
| [wndenv.c](../../LEGOLAND/wndenv.c) | partial | Partial; source gap: full 59-entry character map and cheat strings remain external |

## Supplementary references

The26 lane notes supplement the C evidence. The numbered source tables above count C files only. Where a lane summary disagrees with a later declaration or consumer, the subsystem page preserves and evaluates the disagreement.

- [codex-a.md](../lanes/codex-a.md)
- [codex-b.md](../lanes/codex-b.md)
- [codex-c.md](../lanes/codex-c.md)
- [codex-d.md](../lanes/codex-d.md)
- [fable-a-bswater.md](../lanes/fable-a-bswater.md)
- [fable-a-goldrush2.md](../lanes/fable-a-goldrush2.md)
- [fable-a-jcroute.md](../lanes/fable-a-jcroute.md)
- [fable-a-objrect.md](../lanes/fable-a-objrect.md)
- [fable-a.md](../lanes/fable-a.md)
- [fable-b-ridemisc.md](../lanes/fable-b-ridemisc.md)
- [fable-b-savechunks2.md](../lanes/fable-b-savechunks2.md)
- [fable-b-sysmisc.md](../lanes/fable-b-sysmisc.md)
- [fable-b-workorder3.md](../lanes/fable-b-workorder3.md)
- [fable-b.md](../lanes/fable-b.md)
- [fable-c-data3save.md](../lanes/fable-c-data3save.md)
- [fable-c-mapscreen4.md](../lanes/fable-c-mapscreen4.md)
- [fable-c-sysmisc2.md](../lanes/fable-c-sysmisc2.md)
- [fable-c-workorder4.md](../lanes/fable-c-workorder4.md)
- [fable-c.md](../lanes/fable-c.md)
- [fable-d-audio5.md](../lanes/fable-d-audio5.md)
- [fable-d-coaster7.md](../lanes/fable-d-coaster7.md)
- [fable-d-render5.md](../lanes/fable-d-render5.md)
- [fable-d-uimisc2.md](../lanes/fable-d-uimisc2.md)
- [fable-d.md](../lanes/fable-d.md)
- [scope-e.md](../lanes/scope-e.md)
- [scope-i.md](../lanes/scope-i.md)

Other references: [ride callback recovery](../RIDE_CALLBACKS.md), [formats](../FORMATS.md), [binaries and host imports](../BINARIES.md), [early reverse-engineering context](../RE_CONTEXT.md), [installer archive](../INSTALLSHIELD_Z.md), [LoadBaseMap integration](../DECOMP.md#loadbasemap-interface-for-host--wasm-integration), [LLIDB database](../DECOMP.md#llidb-image-database-asset-resolution), [exported global names](../DECOMP.md#global-names-from-the-export-table).
