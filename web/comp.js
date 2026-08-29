// LEGOLAND COMP sprite decoder (browser port of tools/comp.py).
//
// Reverse-engineered from legoland.exe's software blitter: __BMPLoader
// @0x0044e010 loads the block; the type-3 (16-bit RGB555) per-row decoder at
// 0x004673f0/0x00467d10 runs the opcode loop reimplemented here. Verified
// byte-for-byte against the game's real sprites (the decode matches the
// shipped title/UI/park art pixel-for-pixel).
//
// COMP block: "COMP", u32 w,h,bpp,count,reserved, u32 s0,s1,s2,s3, payload.
//   pixels : 2*s1 bytes  RGB555 literal words
//   length : s2   bytes  run-length bytes
//   control: remainder   2-bit opcodes, 16 per u32, LSB-first (rotating mask)
//
// Exposes window.LLComp.decode(arrayBuffer, offset) -> {width,height,rgba}.

(function () {
  'use strict';

  function rgb555(v, out, o) {
    var r = (v >> 10) & 0x1f, g = (v >> 5) & 0x1f, b = v & 0x1f;
    out[o] = (r << 3) | (r >> 2);
    out[o + 1] = (g << 3) | (g >> 2);
    out[o + 2] = (b << 3) | (b >> 2);
    out[o + 3] = 255;
  }

  // 2-bit opcode reader: 16 codes per u32, LSB-first, rotating 2-bit mask.
  function Bits2(dv, ptr) { this.dv = dv; this.ptr = ptr; this.mask = 3; }
  Bits2.prototype.next = function () {
    var word = this.dv.getUint32(this.ptr, true);
    var code = word & this.mask;
    var hi = (code & 0xAAAAAAAA) !== 0 ? 1 : 0;
    var lo = (code & 0x55555555) !== 0 ? 1 : 0;
    var m = ((this.mask << 2) | (this.mask >>> 30)) >>> 0;
    this.mask = m;
    if (m & 1) this.ptr += 4;
    return (hi << 1) | lo;   // 0b hi lo
  };

  function decode(buffer, off) {
    off = off || 0;
    var dv = new DataView(buffer);
    var u8 = new Uint8Array(buffer);
    if (!(u8[off] === 0x43 && u8[off + 1] === 0x4f && u8[off + 2] === 0x4d && u8[off + 3] === 0x50))
      throw new Error('not a COMP block at ' + off);
    var w = dv.getUint32(off + 4, true), h = dv.getUint32(off + 8, true);
    var bpp = dv.getUint32(off + 12, true);
    var s1 = dv.getUint32(off + 28, true), s2 = dv.getUint32(off + 32, true);
    if (bpp !== 16) throw new Error('unsupported bpp ' + bpp + ' (only 16-bit RGB555 present in shipped data)');

    var base = off + 0x28;
    var pix = base;                 // 2*s1 bytes
    var lenp = base + 2 * s1;       // s2 bytes
    var ctrl = base + 2 * s1 + s2;  // remainder

    var bits = new Bits2(dv, ctrl);
    var rgba = new Uint8ClampedArray(w * h * 4);   // zero => transparent
    var x = 0, y = 0;
    var guard = 0, limit = w * h * 4 + 4096;

    function put(word) { if (x >= 0 && x < w && y < h) rgb555(word, rgba, (y * w + x) * 4); }

    while (y < h) {
      if (++guard > limit) break;
      var code = bits.next();       // 0=00, 1=01, 2=10, 3=11
      if (code < 2) {               // literal 1 pixel (codes 00 / 01)
        put(dv.getUint16(pix, true)); pix += 2; x++;
      } else if (code === 2) {      // transparent 1 pixel
        x++;
      } else {                      // escape
        var L = u8[lenp++];
        if (L === 0) { y++; x = 0; }
        else {
          var sub = bits.next();
          if (sub >= 2) {           // sub hi=1 -> transparent run
            x += L;
          } else if (sub === 0) {   // literal run
            for (var i = 0; i < L; i++) { put(dv.getUint16(pix, true)); pix += 2; x++; }
          } else {                  // fill run
            var word = dv.getUint16(pix, true); pix += 2;
            for (var j = 0; j < L; j++) { put(word); x++; }
          }
        }
      }
    }
    return { width: w, height: h, rgba: rgba };
  }

  // Required API: decodeCOMP(arrayBuffer, offset) -> {width,height,rgba}.
  function decodeCOMP(arrayBuffer, offset) { return decode(arrayBuffer, offset); }
  if (typeof window !== 'undefined') {
    window.decodeCOMP = decodeCOMP;
    window.LLComp = { decode: decode, decodeCOMP: decodeCOMP };
  }
  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { decodeCOMP: decodeCOMP, decode: decode };
  }
})();
