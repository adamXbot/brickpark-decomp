// LEGOLAND tile-set resolver (browser).
//
// Resolves a level's tiles to real .lls sprites:
//   level.tsm_mapping ("EXPLORER MAPPING") -> "<name>.TSM" in Legoland.res
//     -> tile-set name(s) ("EXPLORER SAND SET") -> "<name>.TSF"
//        -> { tile_code -> image.lls }
//   level.terrain ("NEW WESTERN TERRAIN") -> "<name>.ILF" -> ordered tile images
//   level.path_tilesets ("NORMAL PATH TILES") -> via the ICM -> "<file>.TSF"
//
// The .lls tile sprites themselves live in Graphics2.res (ground/path tiles are
// 32x16, a 2:1 isometric diamond; GetTileDimensions confirms width = 2*height).
// Formats reverse-engineered from LLIDB_LoadTSMData/TSFData/ILFData +
// LLIDB_LoadICM (see docs/FORMATS.md).
//
// Exposes window.LLTiles.{ indexRes, parseTSM, parseTSF, parseILF, parseICM,
//   buildResolver }.

(function () {
  'use strict';

  // Recover {lowername: {name, off, size}} for every FILE leaf in a .res.
  function indexRes(bytes) {
    var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    var diroff = dv.getUint32(0, true);
    var idx = {}, n = bytes.length;
    for (var p = diroff; p < n - 20; p++) {
      if (bytes[p] === 0xff && bytes[p + 1] === 0xff && bytes[p + 2] === 0xff && bytes[p + 3] === 0xff) {
        var zero = dv.getUint32(p + 8, true), size = dv.getUint32(p + 12, true), off = dv.getUint32(p + 16, true);
        if (zero === 0 && off > 0 && off < n && size > 0 && size < n) {
          var e = p + 20; while (e < n && bytes[e] !== 0) e++;
          var name = '';
          for (var i = p + 20; i < e; i++) name += String.fromCharCode(bytes[i]);
          if (name && /^[\x20-\x7e]+$/.test(name)) { idx[name.toLowerCase()] = { name: name, off: off, size: size }; p = e; }
        }
      }
    }
    return idx;
  }

  function rdU32(b, o) { return (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)) >>> 0; }
  function rdStr(b, o) { var n = rdU32(b, o); var s = ''; for (var i = 0; i < n; i++) s += String.fromCharCode(b[o + 4 + i]); return { s: s, next: o + 4 + n }; }

  // .TSM: u32 count; count * { str mappingName; str tileSetName }.
  function parseTSM(b) {
    var o = 0, count = rdU32(b, o); o += 4;
    var r = [];
    for (var i = 0; i < count && o < b.length; i++) {
      var a = rdStr(b, o); o = a.next;
      var t = rdStr(b, o); o = t.next;
      r.push({ mapping: a.s, tileset: t.s });
    }
    return r;
  }

  // .TSF: u32 n; str name; n*{u32 code,u32}; n*str image.
  function parseTSF(b) {
    var o = 0, n = rdU32(b, o); o += 4;
    var nm = rdStr(b, o); o = nm.next;
    var codes = [];
    for (var i = 0; i < n; i++) { codes.push(rdU32(b, o)); o += 8; }
    var imgs = [];
    for (i = 0; i < n; i++) { var s = rdStr(b, o); o = s.next; imgs.push(s.s); }
    return { name: nm.s, n: n, codes: codes, images: imgs };
  }

  // .ILF: u16 n; u16 type; str name; then n length-prefixed image names
  // (a fixed per-image record precedes the name table; we recover the names by
  // scanning length-prefixed ascii, which is order-stable).
  function parseILF(b) {
    var n = b[0] | (b[1] << 8);
    var type = b[2] | (b[3] << 8);
    var nm = rdStr(b, 4);
    var names = [];
    var o = nm.next;
    while (o + 4 <= b.length && names.length < n) {
      var len = rdU32(b, o);
      if (len >= 4 && len <= 40 && o + 4 + len <= b.length) {
        var ok = true, s = '';
        for (var i = 0; i < len; i++) { var c = b[o + 4 + i]; if (c < 0x20 || c > 0x7e) { ok = false; break; } s += String.fromCharCode(c); }
        if (ok && s.indexOf('.') > 0) { names.push(s); o += 4 + len; continue; }
      }
      o++;
    }
    return { name: nm.s, n: n, type: type, images: names };
  }

  // .ICM (Legoland.icm): BUILD MENU master index -> {logical: filename}.
  // Layout: header + records, then paired length-prefixed (label, filename)
  // strings. We recover the pairs by scanning length-prefixed ascii and taking
  // consecutive (label, filename-with-extension) pairs.
  function parseICM(b) {
    var map = {}, o = 0, pend = null;
    while (o + 4 <= b.length) {
      var len = rdU32(b, o);
      if (len >= 2 && len <= 48 && o + 4 + len <= b.length) {
        var ok = true, s = '';
        for (var i = 0; i < len; i++) { var c = b[o + 4 + i]; if (c < 0x20 || c > 0x7e) { ok = false; break; } s += String.fromCharCode(c); }
        if (ok && /^[A-Za-z0-9]/.test(s)) {
          if (pend && /\.[A-Za-z0-9]{2,4}$/.test(s)) { map[pend.toUpperCase()] = s; pend = null; }
          else pend = s;
          o += 4 + len; continue;
        }
      }
      pend = null; o++;
    }
    return map;
  }

  // .ODF (object definition): binary header + ascii strings. We extract the
  // object's main sprite: the first ".LLS"/".CSP" string that isn't an ICON or
  // a build-up (_BU / "BUILD UP") variant. Also returns the icon name.
  function parseODF(b) {
    var strs = [], i = 0, n = b.length;
    while (i < n) {
      if (b[i] >= 0x20 && b[i] < 0x7f) {
        var j = i; while (j < n && b[j] >= 0x20 && b[j] < 0x7f) j++;
        if (j - i >= 3) strs.push(String.fromCharCode.apply(null, b.subarray(i, j)));
        i = j;
      } else i++;
    }
    var sprite = null, icon = null;
    for (var k = 0; k < strs.length; k++) {
      var s = strs[k], up = s.toUpperCase();
      if (/\.(LLS|CSP)$/.test(up)) {
        if (up.indexOf('ICON') >= 0) { if (!icon) icon = s; continue; }
        if (up.indexOf('_BU') >= 0 || up.indexOf('BUILD UP') >= 0) continue;
        if (!sprite) sprite = s;
      }
    }
    return { sprite: sprite, icon: icon, strings: strs };
  }

  // .CSP (composite sprite): u16 n; u16 type; str name; n*{s32 dx,s32 dy};
  // n*str image.lls. The parts layer with per-part pixel offsets to build a
  // large object (e.g. FORT = fort3/fort2/fort1.lls).
  function parseCSP(b) {
    var n = b[0] | (b[1] << 8);
    var nm = rdStr(b, 4);
    var o = nm.next;
    var parts = [];
    for (var i = 0; i < n; i++) {
      var dx = rdU32(b, o) | 0, dy = rdU32(b, o + 4) | 0;   // signed
      parts.push({ dx: dx, dy: dy }); o += 8;
    }
    for (i = 0; i < n && o < b.length; i++) {
      var s = rdStr(b, o); o = s.next; if (parts[i]) parts[i].image = s.s;
    }
    return { name: nm.s, n: n, parts: parts };
  }

  window.LLTiles = { indexRes: indexRes, parseTSM: parseTSM, parseTSF: parseTSF, parseILF: parseILF, parseICM: parseICM, parseODF: parseODF, parseCSP: parseCSP };
})();
