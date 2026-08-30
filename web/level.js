// LEGOLAND .MAP level parser (browser port of tools/leveldata.py).
//
// The 17 shipped park maps live inside Legoland.res as `*.MAP` members. Format
// confirmed against LoadBaseMap @ VA 0x00461a50; every map decodes to EOF with
// zero leftover bytes. See docs/FORMATS.md.
//
// Exposes window.LLLevel.parse(uint8) -> { name, tsm_mapping, terrain, width,
//   height, object_classes, objects:[{cls,x,y}], path_tilesets, layers,
//   terrainGrid: Int32Array(w*h) of terrain index (-1 = transparent/base),
//   terrainDelta: Uint8Array(w*h), tileGfx: Int32Array(w*h) (-1 = none) }.

(function () {
  'use strict';

  function Reader(b) { this.b = b; this.o = 0; }
  Reader.prototype.u8 = function () { return this.b[this.o++]; };
  Reader.prototype.u16 = function () { var v = this.b[this.o] | (this.b[this.o + 1] << 8); this.o += 2; return v; };
  Reader.prototype.u32 = function () { var v = (this.b[this.o] | (this.b[this.o + 1] << 8) | (this.b[this.o + 2] << 16) | (this.b[this.o + 3] << 24)) >>> 0; this.o += 4; return v; };
  Reader.prototype.s = function () { var n = this.u32(); var s = ''; for (var i = 0; i < n; i++) s += String.fromCharCode(this.b[this.o + i]); this.o += n; return s; };
  Reader.prototype.eof = function () { return this.o >= this.b.length; };

  // Decode one of the four 2-bit-tagged RLE grid layers into an Int32Array.
  // Control byte: n = b & 0x3f, mode = b & 0xc0. mode 0x00 = skip n cells
  // (leave -1); mode 0x40 = run of n cells all equal to next byte value;
  // mode 0x80/0xc0 = n literal cell bytes. Row-major, fills w*h.
  function decodeLayer(b, off, size, w, h) {
    var out = new Int32Array(w * h).fill(-1);
    var end = off + size, p = off, i = 0, total = w * h;
    while (p < end && i < total) {
      var c = b[p++];
      var n = c & 0x3f, mode = c & 0xc0;
      if (n === 0) n = 64;
      if (mode === 0x00) { i += n; }
      else if (mode === 0x40) { var v = b[p++]; for (var k = 0; k < n && i < total; k++) out[i++] = v; }
      else { for (var j = 0; j < n && i < total && p < end; j++) out[i++] = b[p++]; }
    }
    return out;
  }

  function parse(b) {
    var r = new Reader(b), m = {};
    m.name = r.s();
    m.tsm_mapping = r.s();
    m.terrain = r.s();
    m.width = r.u16();
    m.height = r.u16();
    var w = m.width, h = m.height;

    var nc = r.u32(); m.object_classes = [];
    for (var i = 0; i < nc; i++) m.object_classes.push(r.s());
    var no = r.u32(); m.objects = [];
    for (i = 0; i < no; i++) m.objects.push({ cls: r.u32(), x: r.u32(), y: r.u32() });
    var nps = r.u32(); m.path_tilesets = [];
    for (i = 0; i < nps; i++) m.path_tilesets.push(r.s());

    var layerNames = ['tile_gfx', 'map_flags', 'rf_flags', 'user_flags'];
    m.layers = {};
    for (i = 0; i < layerNames.length; i++) {
      var sz = r.u32();
      m.layers[layerNames[i]] = { off: r.o, size: sz };
      r.o += sz;
    }
    m.tileGfx = decodeLayer(b, m.layers.tile_gfx.off, m.layers.tile_gfx.size, w, h);

    // n_extra 20-byte terrain render-object records (sub_462c00): the perimeter
    // cliff/edge sprites. Each = {i32 x, i32 y, i32 x2(=2x), i32 y2(=2y), u32 image}.
    // x,y are iso screen coords (left=16*(x-y-1), top=8*(x+y)); image indexes the
    // terrain .ILF; (image>>8)&0xff selects the bridge ILF instead.
    m.n_extra = r.u32();
    m.extra = [];
    for (var ei = 0; ei < m.n_extra; ei++) {
      var ex = r.b[r.o] | (r.b[r.o + 1] << 8) | (r.b[r.o + 2] << 16) | (r.b[r.o + 3] << 24);
      var ey = r.b[r.o + 4] | (r.b[r.o + 5] << 8) | (r.b[r.o + 6] << 16) | (r.b[r.o + 7] << 24);
      var eimg = (r.b[r.o + 16] | (r.b[r.o + 17] << 8) | (r.b[r.o + 18] << 16) | (r.b[r.o + 19] << 24)) >>> 0;
      m.extra.push({ x: ex | 0, y: ey | 0, image: eimg & 0xff, bridge: (eimg >> 8) & 0xff });
      r.o += 20;
    }

    m.has_bridges = false;
    if (r.o + 8 <= b.length && String.fromCharCode.apply(null, b.subarray(r.o, r.o + 8)) === 'BRIDGES!') {
      r.o += 8; m.has_bridges = true; m.bridge_terrain = r.s();
    }

    // terrain-tile stream state machine -> per-cell (terrain idx, delta)
    var grid = new Int32Array(w * h).fill(-1);
    var delta = new Uint8Array(w * h);
    var state = 0, counter = 0, cells = 0, row = 0, done = false;
    while (row < h && !done) {
      var col = 0;
      while (col < w) {
        if (state === 0) {
          if (r.eof()) { done = true; break; }
          var c = r.u8();
          state = (c === 0) ? 1 : 2; counter = c; continue;
        }
        if (state === 2) {
          if (counter !== 0) { counter--; col++; } else state = 1;
          continue;
        }
        if (r.o + 2 > b.length) { done = true; break; }
        var code = r.u16();
        if (code === 0xffff) { state = 0; col++; }
        else { var idx = row * w + col; grid[idx] = (code >> 8) & 0xff; delta[idx] = code & 0xff; cells++; col++; }
      }
      row++;
    }
    m.terrainGrid = grid;
    m.terrainDelta = delta;
    m.terrain_cells = cells;
    m.leftover = b.length - r.o;
    m._buf = b;   // keep the raw .MAP bytes so the tile resolver can read the RLE layers
    return m;
  }

  window.LLLevel = { parse: parse, decodeLayer: decodeLayer };
})();
