// LEGOLAND per-cell tile resolver (browser port of tools/tilemap.py).
//
// Replays the engine's cell->sprite pipeline (LoadBaseMap 0x461a50 tile_gfx RLE
// + flag layers + path autotiler). For each grid cell it yields the drawn .lls
// sprite: the tile_gfx ground tile (TSF[group].images[delta]), or a path cell's
// neighbour-autotiled OUTLnn sprite, plus the terrain-stream base plane.
//
// The tile_gfx RLE (0x461e66) carries a 1-byte accumulator selecting a tile
// GROUP; mode 0x00 SETs it, 0x40 = literal run, 0x80 = fill run, 0xc0 = empty.
// acc&0x20 => object/path cell (pathset acc&0x1f); else ground idx = acc-1,
// delta = per-cell data byte. See docs/FORMATS.md + scratchpad/slope_re/*.md.
//
// Exposes window.LLTilemap.resolve(m, getMember) -> per-cell arrays.

(function () {
  'use strict';

  var OUTL_LOCAL_BASE = 3; // NORMPATH image index of OUTL00 (0..2 = PATH15/BLU/RED)

  // tile_gfx RLE: acc-based. Returns per-cell {kind,group,delta}|{kind:'object',pathset}|{kind:'empty'}|null
  function decodeTileGfx(buf, w, h) {
    var total = w * h, out = new Array(total).fill(null);
    var edi = 2, acc = 0, cur = 0;   // first 2 bytes reserved (0x461ea8)
    while (cur < total && edi < buf.length) {
      var b = buf[edi++]; var n = b & 0x3f, mode = b & 0xc0;
      if (mode !== 0 && n === 0) n = 64;
      if (mode === 0x00) { acc = n; }
      else if (mode === 0x40 || mode === 0x80) {
        for (var i = 0; i < n && cur < total; i++) {
          var db = buf[edi];
          if (acc & 0x20) out[cur] = { kind: 'object', pathset: acc & 0x1f };
          else out[cur] = { kind: 'ground', group: (acc - 1) & 0xff, delta: db };
          cur++;
          if (mode === 0x40) edi++;
        }
        if (mode === 0x80) edi++;
      } else { // 0xc0 empty
        for (var j = 0; j < n && cur < total; j++) { out[cur] = { kind: 'empty' }; cur++; }
      }
    }
    return out;
  }

  // flag layer RLE: no accumulator, edi=0, only 0x40 literal / 0x80 fill emit.
  function decodeFlags(buf, w, h) {
    var total = w * h, out = new Uint8Array(total), edi = 0, cur = 0;
    while (cur < total && edi < buf.length) {
      var b = buf[edi++]; var n = b & 0x3f, mode = b & 0xc0;
      if (n === 0) n = 64;
      if (mode === 0x40) { for (var i = 0; i < n && cur < total; i++) out[cur++] = buf[edi++]; }
      else if (mode === 0x80) { var db = buf[edi]; for (var j = 0; j < n && cur < total; j++) out[cur++] = db; edi++; }
    }
    return out;
  }

  function resolve(m, getMember) {
    var w = m.width, h = m.height, total = w * h;
    var T = window.LLTiles;

    // .TSM = u32 count; str name; count* str tilesetName (matches LLIDB_LoadTSMData)
    function parseTSMsubs(b) {
      var o = 0;
      var count = (b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)) >>> 0; o = 4;
      function str() { var n = (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)) >>> 0; o += 4; var s = ''; for (var i = 0; i < n; i++) s += String.fromCharCode(b[o + i]); o += n; return s; }
      str(); // TSM name
      var subs = [];
      for (var i = 0; i < count; i++) subs.push(str());
      return subs;
    }
    // ground TSF group table (level's tsm_mapping -> its sub TSFs, in order)
    var tsmRecs = [];
    var tsmB = getMember(m.tsm_mapping + '.TSM');
    if (tsmB) {
      parseTSMsubs(tsmB).forEach(function (sub) {
        var tb = getMember(sub + '.TSF');
        tsmRecs.push(tb ? T.parseTSF(tb) : null);
      });
    }
    // path autotile set (NORMPATH). Levels reference "NORMAL PATH TILES".
    var pathTsf = null;
    var pb = getMember('NORMPATH.TSF');
    if (!pb && m.path_tilesets[0]) pb = getMember(m.path_tilesets[0] + '.TSF');
    if (pb) pathTsf = T.parseTSF(pb);

    // decode layers straight from the parsed .MAP buffer
    var buf = m._buf;   // set by caller (the raw .MAP bytes)
    function layer(name) {
      var L = m.layers[name];
      return buf.subarray(L.off, L.off + L.size);
    }
    var tg = decodeTileGfx(layer('tile_gfx'), w, h);
    var mapflags = decodeFlags(layer('map_flags'), w, h);
    var rfflags = decodeFlags(layer('rf_flags'), w, h);

    // path predicate (0x45ce10): rf bit0 forces path; else map_flags&0x10 & !rf bit1
    var isPath = new Uint8Array(total);
    for (var i = 0; i < total; i++) {
      var rf = rfflags[i];
      if (rf & 1) isPath[i] = 1;
      else if ((mapflags[i] & 0x10) && !(rf & 2)) isPath[i] = 1;
    }
    function P(x, y) { return (x >= 0 && x < w && y >= 0 && y < h) ? isPath[y * w + x] : 0; }

    var image = new Array(total).fill(null);   // primary drawn sprite name
    var overlay = new Array(total).fill(null); // concave path overlay name
    var terrain = new Array(total).fill(null); // terrain-stream base plane name

    for (var y = 0; y < h; y++) {
      for (var x = 0; x < w; x++) {
        var idx = y * w + x, c = tg[idx];
        if (c && c.kind === 'ground' && c.group >= 0 && c.group < tsmRecs.length) {
          var tsf = tsmRecs[c.group];
          if (tsf && c.delta < tsf.images.length) image[idx] = tsf.images[c.delta];
        }
        if (isPath[idx] && pathTsf) {
          var mm = (P(x, y - 1) ? 1 : 0) | (P(x + 1, y) ? 2 : 0) | (P(x, y + 1) ? 4 : 0) | (P(x - 1, y) ? 8 : 0);
          var pi = OUTL_LOCAL_BASE + mm;
          if (pi < pathTsf.images.length) image[idx] = pathTsf.images[pi];
          var cc = 0;
          if ((mm & 0x0C) === 0x0C && !P(x - 1, y + 1)) cc |= 1;
          if ((mm & 0x03) === 0x03 && !P(x + 1, y - 1)) cc |= 2;
          if (cc) { var oi = OUTL_LOCAL_BASE + 15 + cc; if (oi < pathTsf.images.length) overlay[idx] = pathTsf.images[oi]; }
        }
      }
    }

    // terrain-stream base plane (cell+0xa): idx=hi-1 into the SAME tsm records, delta=lo
    var tgrid = m.terrainGrid, tdelta = m.terrainDelta;
    for (var k = 0; k < total; k++) {
      var hi = tgrid[k];
      if (hi >= 0) {
        var ti = hi - 1;
        if (ti >= 0 && ti < tsmRecs.length && tsmRecs[ti]) {
          var lo = tdelta[k];
          if (lo < tsmRecs[ti].images.length) terrain[k] = tsmRecs[ti].images[lo];
        }
      }
    }

    return { image: image, overlay: overlay, terrain: terrain, isPath: isPath,
             tsmRecs: tsmRecs, pathTsf: pathTsf };
  }

  window.LLTilemap = { resolve: resolve, decodeTileGfx: decodeTileGfx, decodeFlags: decodeFlags };
})();
