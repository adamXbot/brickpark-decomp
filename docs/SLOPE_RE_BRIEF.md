# Slope-table / tile-resolution RE brief

GOAL: fully replay how the LEGOLAND engine turns a `.MAP` grid into rendered
sprites, so the browser can paint every cell's EXACT tile (ground slopes,
cliffs, water edges, path variants) instead of falling back to a base ground
tile. Today only tile codes that map 1:1 to a `.TSF` code resolve (~17%); the
rest are isometric corner-height / slope configs the engine resolves through a
`base + delta` tile table in the loaded LLIDB image database.

## The unknown we must nail

Given the loaded tile-sets (TSF/ILF/CSP/ICM in `Legoland.res`) and a parsed
`.MAP`, compute for EACH grid cell the exact sprite(s) drawn and their screen
placement. Concretely, resolve the `tile_gfx` layer byte + the terrain-stream
`(terrain_idx, delta)` to a `.lls` image name (in Graphics1/2.res) and a render
offset.

## Key facts already established (see docs/FORMATS.md, docs/RE_CONTEXT.md)

- `.MAP` parsed by `LoadBaseMap` @ VA 0x00461a50; 4 size-prefixed RLE layers
  (tile_gfx, map_flags, rf_flags, user_flags) + a terrain-tile state machine.
- `SetMapTile(x,y,val)` @0x00461780 stores `val` as a WORD at map cell+8; the
  map row array is at global `[0x00801400]`, cell stride = 20 bytes (5 dwords).
- In LoadBaseMap the tile_gfx loop computes (per cell): a tile-group is looked up
  in a table at `[esp+0x3c]` as `group = table[idx*8 + 4]`, then
  `finalTileWord = *(u16*)group + tileByte`, stored at cell+0xa. `idx` is built
  from the layer byte(s) + a running accumulator. THIS loop is the heart of it.
- `.TSF`: u32 n; str name; n*{u32 code,u32}; n*str image. `.ILF`: u16 n; u16
  type; str name; n images. `.CSP`: u16 n; u16 type; str name; n*{s32 dx,s32 dy};
  n*str image. Ground/path tiles are 32x16 (2:1 iso; `GetTileDimensions`
  @0x00460540: width = 2*height).

## Function map (VA = RVA + 0x400000; pass RVA to tools/disasm.py)

RENDER PATH (the ground truth for cell -> sprite):
- RenderFullMap        RVA 0x567a0   <-- THE map draw loop; how each cell is resolved+drawn
- CalculateMapRenderOrder RVA 0x5a4a0
- RenderTiledSprite    RVA 0x88c50
- GetSpriteForLayer    RVA 0x41ec0   GetLLSForLayer 0x41ea0   GetRenderOffsetForLayer 0x41ee0
- GetLayer 0x97e80     TileSpriteArray 0x405f60   TileSpriteInfo 0x401f40
- GameMap (global) VA 0x00401400 ; map row array global VA 0x00801400

MAP BUILD:
- LoadBaseMap 0x61a50   LoadMapTiles 0x5aad0   PutObjOnMap 0x59ad0
- SetMapTile 0x61780    SetMapFlags 0x61810    Set_RFFlags 0x616e0
- GetTileDimensions 0x60540  GetTileCentre 0x5ad60  GetTileBounds 0x5acc0  GetTileInDir 0x846a0

LLIDB IMAGE DATABASE:
- LLIDB_LoadData 0x7d3a0   LLIDB_RegisterNewElement 0x7b610  LLIDB_RegisterNewElementB 0x7b860
- LLIDB_FindElement 0x7b330  LLIDB_FindElementFromDataPtr 0x7b410  LLIDB_GetElement 0x7b2e0
- LLIDB_GetCount 0x7b2d0   LLIDB_SelectElement 0x7bc20   LLIDB_ClearOnLevel 0x7b4c0
- LLIDB_LoadICM 0x7aff0    LLIDB_LoadTSFData 0x7cba0   LLIDB_LoadILFData 0x7cfc0
- LLIDB_LoadCSPData 0x7d1a0  LLIDB_LoadODFData 0x7bf70  LLIDB_LoadTSMData 0x7ce40

PATH TILES (variant selection by neighbours):
- AddPathTile 0x5d3b0  AddPathTileGFX 0x5d350  AdjustPathTile 0x5d1a0  RemovePathTile 0x5daa0

## Deliverable per reader

A precise, disassembly-grounded spec of your function(s): struct/field offsets,
exact arithmetic, globals touched, and how it contributes to `cell -> sprite`.
Where feasible, a short Python snippet validated against the real bytes in
gamedata/. Do NOT guess; cite instruction addresses. The synthesis stage will
combine all specs into one implementable resolver.
