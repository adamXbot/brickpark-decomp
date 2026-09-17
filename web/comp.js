// LEGOLAND COMP sprite decoder (browser port of tools/comp.py).
//
// A COMP block is an .lls member of Graphics1.res / Graphics2.res: the LLS
// animation record that __BMPLoader @0x0044e010 loads whole. Its FORMAT word
// picks one of two frame encodings, each reimplemented from the game's own
// software blitters.
//
// COMP block: "COMP", u32 w,h,format, u16 count, u16 timer, u32 flags, then the
// frame table at +0x18: length-linked records, each starting with its i32 size.
// flags bit 0 puts a base image record first, so the table holds count+1.
//
// Format 0x10, 16-bit RGB555 RLE (record 0 only): the type-3 per-row decoder at
// 0x004673f0/0x00467d10 runs the opcode loop reimplemented here. Verified
// byte-for-byte against the game's real sprites (the decode matches the
// shipped title/UI/park art pixel-for-pixel). Record 0 is u32 s0,s1,s2,s3,
// then the payload:
//   pixels : 2*s1 bytes  RGB555 literal words
//   length : s2   bytes  run-length bytes
//   control: remainder   2-bit opcodes, 16 per u32, LSB-first (rotating mask)
//
// Format 8, 8-bit paletted frames (any record): the type-2 records that
// SoftBlitAnimPlain @0x00464480 and SoftBlitAnim @0x00465240 paint. A record is
// i32 size, i32 npixels, npixels index bytes, then (record 0 only) a 0x200-byte
// palette of 256 RGB555 words that every record uses, then a u32 control stream
// to the end of the record (Ctl8, decode8). Matches tools/comp.py's RGBA and
// stream consumption on every frame record of Graphics2.res's 365 members.
//
// Exposes window.LLComp.decode(arrayBuffer, offset, frame) -> {width,height,rgba}
// (frame: the frame record, default 0; no base image is composited) and
// window.LLComp.frames8(arrayBuffer, offset) -> the format-8 frame table.

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

  // Format-8 control reader: ll_anim_code / ll_anim_count (ll_portable.h), the
  // C form of the reader both type-2 painters spell in asm. It shifts the word
  // it holds right by 2 before each code and loads the next word once all 16
  // codes are used. `end` bounds the stream, so a bad walk throws rather than
  // reading the next record.
  function Ctl8(dv, ptr, end) { this.dv = dv; this.ptr = ptr; this.end = end; this.cur = 0; this.bits = 0; }
  Ctl8.prototype.load = function () {
    if (this.ptr + 4 > this.end) throw new Error('control stream overrun at 0x' + this.ptr.toString(16));
    this.cur = this.dv.getUint32(this.ptr, true);
    this.ptr += 4;
  };
  Ctl8.prototype.code = function () {
    this.cur >>>= 2;                   // the shift happens even on a reload
    if (--this.bits < 0) { this.bits = 15; this.load(); }
    return this.cur & 3;
  };
  // The 8-bit count after an 11 code fills four code slots: the low byte of the
  // word just shifted when at least four slots are left in it, else the low
  // byte of a freshly loaded word (the old word's remainder is dropped).
  Ctl8.prototype.count = function () {
    this.cur >>>= 2;
    this.bits -= 4;
    if (this.bits < 0) { this.bits = 12; this.load(); }
    var n = this.cur & 0xff;
    this.cur >>>= 6;
    return n;
  };

  function isComp(u8, off) {
    return u8[off] === 0x43 && u8[off + 1] === 0x4f && u8[off + 2] === 0x4d && u8[off + 3] === 0x50;
  }

  function decode(buffer, off, frame) {
    off = off || 0;
    if (frame == null) frame = 0;
    var dv = new DataView(buffer);
    var u8 = new Uint8Array(buffer);
    if (!isComp(u8, off))
      throw new Error('not a COMP block at ' + off);
    var w = dv.getUint32(off + 4, true), h = dv.getUint32(off + 8, true);
    var format = dv.getUint32(off + 12, true);
    if (format === 8) return decode8(buffer, dv, u8, off, w, h, frame);
    if (format !== 16) throw new Error('unsupported format ' + format);
    if (frame !== 0) throw new Error('the 16-bpp reader decodes record 0 only');
    var s1 = dv.getUint32(off + 28, true), s2 = dv.getUint32(off + 32, true);

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

  // The frame table of a format-8 block: one entry per record, offsets
  // absolute: {offset, size, npixels, index, palette, ctrl, end}. `palette` is
  // null except on record 0, whose palette sits between its index block and its
  // control stream.
  function frames8(buffer, off) {
    off = off || 0;
    var dv = new DataView(buffer);
    var n = buffer.byteLength;
    if (!isComp(new Uint8Array(buffer), off))
      throw new Error('not a COMP block at ' + off);
    if (dv.getUint32(off + 12, true) !== 8) throw new Error('not a format-8 COMP block');
    var records = dv.getUint16(off + 16, true) + (dv.getUint32(off + 20, true) & 1);
    var recs = [], p = off + 0x18;
    for (var i = 0; i < records; i++) {
      if (p + 8 > n) throw new Error('frame record ' + i + ' at 0x' + p.toString(16) + ' is past the data');
      var size = dv.getInt32(p, true), npixels = dv.getInt32(p + 4, true);
      var index = p + 8;
      var ctrl = index + npixels + (i === 0 ? 0x200 : 0);
      var end = p + size;
      if (npixels < 0 || end < ctrl || (end - ctrl) % 4 || end > n)
        throw new Error('frame record ' + i + ' at 0x' + p.toString(16) + ' is malformed (size ' +
                        size + ', npixels ' + npixels + ')');
      recs.push({ offset: p, size: size, npixels: npixels, index: index,
                  palette: i === 0 ? index + npixels : null, ctrl: ctrl, end: end });
      p = end;
    }
    return recs;
  }

  // Format 8: frame record `frame`, coloured through record 0's palette.
  //   bit1 = 0  one literal pixel (next index byte)   [codes 00 and 01]
  //   10        one transparent pixel
  //   11        an 8-bit count follows (Ctl8.count); 0 ends the row, else a
  //             sub-code: bit1 = 1 skips count pixels, 00 copies count index
  //             bytes, 01 repeats one index byte count times
  function decode8(buffer, dv, u8, off, w, h, frame) {
    var recs = frames8(buffer, off);
    if (!(frame >= 0 && frame < recs.length && frame === Math.floor(frame)))
      throw new Error('frame ' + frame + ' is out of range: ' + recs.length + ' frame records');
    var ink = new Uint8Array(256 * 4);             // index -> RGBA
    for (var k = 0; k < 256; k++) rgb555(dv.getUint16(recs[0].palette + 2 * k, true), ink, 4 * k);
    var rec = recs[frame];
    var ip = rec.index, iend = ip + rec.npixels;
    var bits = new Ctl8(dv, rec.ctrl, rec.end);
    var rgba = new Uint8ClampedArray(w * h * 4);   // zero => transparent
    var x = 0, y = 0, row = 0;                     // row: rgba offset of row y

    function put(o, v) {
      v *= 4;
      rgba[o] = ink[v]; rgba[o + 1] = ink[v + 1]; rgba[o + 2] = ink[v + 2]; rgba[o + 3] = ink[v + 3];
    }

    while (y < h) {
      var code = bits.code();
      if (!(code & 2)) {                           // one literal pixel (00 / 01)
        if (ip >= iend) throw new Error('index block overrun in row ' + y);
        if (x < w) put(row + 4 * x, u8[ip]);
        ip++; x++;
      } else if (!(code & 1)) {                    // 10: one transparent pixel
        x++;
      } else {                                     // 11: a count follows
        var n = bits.count();
        if (n === 0) { y++; x = 0; row += 4 * w; continue; }   // end of row
        var sub = bits.code();
        if (sub & 2) { x += n; continue; }         // skip n pixels
        var take = (sub & 1) ? 1 : n;              // 01 repeats one byte, 00 copies n
        if (ip + take > iend) throw new Error('index block overrun in row ' + y);
        var vis = Math.min(n, w - x);              // clipped at the right edge
        for (var i = 0; i < vis; i++) put(row + 4 * (x + i), u8[(sub & 1) ? ip : ip + i]);
        ip += take; x += n;
      }
    }
    return { width: w, height: h, rgba: rgba, frame: frame, records: recs.length,
             consumed: { index_bytes: ip - rec.index, control_bytes: bits.ptr - rec.ctrl } };
  }

  // Required API: decodeCOMP(arrayBuffer, offset) -> {width,height,rgba}.
  function decodeCOMP(arrayBuffer, offset, frame) { return decode(arrayBuffer, offset, frame); }
  if (typeof window !== 'undefined') {
    window.decodeCOMP = decodeCOMP;
    window.LLComp = { decode: decode, decodeCOMP: decodeCOMP, frames8: frames8 };
  }
  if (typeof module !== 'undefined' && module.exports) {
    module.exports = { decodeCOMP: decodeCOMP, decode: decode, frames8: frames8 };
  }
})();
